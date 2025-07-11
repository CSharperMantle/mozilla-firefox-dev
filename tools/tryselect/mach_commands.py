# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.


import argparse
import importlib
import os
import sys

from mach.decorators import Command, SubCommand
from mach.util import get_state_dir
from mozbuild.util import memoize

CONFIG_ENVIRONMENT_NOT_FOUND = """
No config environment detected. This means we are unable to properly
detect test files in the specified paths or tags. Please run:

    $ mach configure

and try again.
""".lstrip()


class get_parser:
    def __init__(self, selector):
        self.selector = selector

    def __call__(self):
        mod = importlib.import_module(f"tryselect.selectors.{self.selector}")
        return getattr(mod, f"{self.selector.capitalize()}Parser")()


def generic_parser():
    from tryselect.cli import BaseTryParser

    parser = BaseTryParser()
    parser.add_argument("argv", nargs=argparse.REMAINDER)
    return parser


def init(command_context):
    from tryselect import lando, push

    mach_context = command_context._mach_context
    lando.LAUNCH_BROWSER = not mach_context.settings["try"]["nobrowser"]
    push.MAX_HISTORY = mach_context.settings["try"]["maxhistory"]


@memoize
def presets(command_context):
    from tryselect.preset import MergedHandler

    # Create our handler using both local and in-tree presets. The first
    # path in this list will be treated as the 'user' file for the purposes
    # of saving and editing. All subsequent paths are 'read-only'. We check
    # an environment variable first for testing purposes.
    if os.environ.get("MACH_TRY_PRESET_PATHS"):
        preset_paths = os.environ["MACH_TRY_PRESET_PATHS"].split(os.pathsep)
    else:
        preset_paths = [
            os.path.join(get_state_dir(), "try_presets.yml"),
            os.path.join(
                command_context.topsrcdir, "tools", "tryselect", "try_presets.yml"
            ),
        ]

    return MergedHandler(*preset_paths)


def handle_presets(
    command_context, preset_action=None, save=None, preset=None, **kwargs
):
    """Handle preset related arguments.

    This logic lives here so that the underlying selectors don't need
    special preset handling. They can all save and load presets the same
    way.
    """
    from tryselect.util.dicttools import merge

    user_presets = presets(command_context).handlers[0]
    if preset_action == "list":
        presets(command_context).list()
        sys.exit()

    if preset_action == "edit":
        user_presets.edit()
        sys.exit()

    parser = command_context._mach_context.handler.parser
    subcommand = command_context._mach_context.handler.subcommand
    if "preset" not in parser.common_groups:
        return kwargs

    default = parser.get_default
    if save:
        selector = (
            subcommand or command_context._mach_context.settings["try"]["default"]
        )

        # Only save non-default values for simplicity.
        kwargs = {k: v for k, v in kwargs.items() if v != default(k)}
        user_presets.save(save, selector=selector, **kwargs)
        print(f"preset saved, run with: --preset={save}")
        sys.exit()

    if preset:
        if preset not in presets(command_context):
            command_context._mach_context.parser.error(
                f"preset '{preset}' does not exist"
            )

        name = preset
        preset = presets(command_context)[name]
        selector = preset.pop("selector")
        preset.pop("description", None)  # description isn't used by any selectors

        if not subcommand:
            subcommand = selector
        elif subcommand != selector:
            print(
                f"error: preset '{name}' exists for a different selector "
                f"(did you mean to run 'mach try {selector}' instead?)"
            )
            sys.exit(1)

        # Order of precedence is defaults -> presets -> cli. Configuration
        # from the right overwrites configuration from the left.
        defaults = {}
        nondefaults = {}
        for k, v in kwargs.items():
            if v == default(k):
                defaults[k] = v
            else:
                nondefaults[k] = v

        kwargs = merge(defaults, preset, nondefaults)

    return kwargs


def handle_try_params(command_context, **kwargs):
    from tryselect.util.dicttools import merge

    to_validate = []
    kwargs.setdefault("try_config_params", {})
    for cls in command_context._mach_context.handler.parser.task_configs.values():
        params = cls.get_parameters(**kwargs)
        if params is not None:
            to_validate.append(cls)
            kwargs["try_config_params"] = merge(kwargs["try_config_params"], params)

        for name in cls.dests:
            del kwargs[name]

    # Validate task_configs after they have all been parsed to avoid
    # depending on the order they were processed.
    for cls in to_validate:
        cls.validate(**kwargs)
    return kwargs


def run(command_context, **kwargs):
    kwargs = handle_presets(command_context, **kwargs)

    if command_context._mach_context.handler.parser.task_configs:
        kwargs = handle_try_params(command_context, **kwargs)

    mod = importlib.import_module(
        f"tryselect.selectors.{command_context._mach_context.handler.subcommand}"
    )
    return mod.run(**kwargs)


@Command(
    "try",
    category="ci",
    description="Push selected tasks to the try server",
    parser=generic_parser,
    virtualenv_name="try",
)
def try_default(command_context, argv=None, **kwargs):
    """Push selected tests to the try server.

    The |mach try| command is a frontend for scheduling tasks to
    run on try server using selectors. A selector is a subcommand
    that provides its own set of command line arguments and are
    listed below.

    If no subcommand is specified, the `auto` selector is run by
    default. Run |mach try auto --help| for more information on
    scheduling with the `auto` selector.
    """
    init(command_context)
    subcommand = command_context._mach_context.handler.subcommand
    # We do special handling of presets here so that `./mach try --preset foo`
    # works no matter what subcommand 'foo' was saved with.
    preset = kwargs["preset"]
    if preset:
        if preset not in presets(command_context):
            command_context._mach_context.handler.parser.error(
                f"preset '{preset}' does not exist"
            )

        subcommand = presets(command_context)[preset]["selector"]

    sub = subcommand or command_context._mach_context.settings["try"]["default"]
    return command_context._mach_context.commands.dispatch(
        "try",
        command_context._mach_context,
        command_site_manager=command_context.virtualenv_manager,
        subcommand=sub,
        argv=argv,
        **kwargs,
    )


@SubCommand(
    "try",
    "fuzzy",
    description="Select tasks on try using a fuzzy finder",
    parser=get_parser("fuzzy"),
    virtualenv_name="try",
)
def try_fuzzy(command_context, **kwargs):
    """Select which tasks to run with a fuzzy finding interface (fzf).

    When entering the fzf interface you'll be confronted by two panes. The
    one on the left contains every possible task you can schedule, the one
    on the right contains the list of selected tasks. In other words, the
    tasks that will be scheduled once you you press <enter>.

    At first fzf will automatically select whichever task is under your
    cursor, which simplifies the case when you are looking for a single
    task. But normally you'll want to select many tasks. To accomplish
    you'll generally start by typing a query in the search bar to filter
    down the list of tasks (see Extended Search below). Then you'll either:

    A) Move the cursor to each task you want and press <tab> to select it.
    Notice it now shows up in the pane to the right.

    OR

    B) Press <ctrl-a> to select every task that matches your filter.

    You can delete your query, type a new one and select further tasks as
    many times as you like. Once you are happy with your selection, press
    <enter> to push the selected tasks to try.

    All selected task labels and their dependencies will be scheduled. This
    means you can select a test task and its build will automatically be
    filled in.


    Keyboard Shortcuts
    ------------------

    When in the fuzzy finder interface, start typing to filter down the
    task list. Then use the following keyboard shortcuts to select tasks:

      Ctrl-K / Up    => Move cursor up
      Ctrl-J / Down  => Move cursor down
      Tab            => Select task + move cursor down
      Shift-Tab      => Select task + move cursor up
      Ctrl-A         => Select all currently filtered tasks
      Ctrl-D         => De-select all currently filtered tasks
      Ctrl-T         => Toggle select all currently filtered tasks
      Alt-Bspace     => Clear query from input bar
      Enter          => Accept selection and exit
      Ctrl-C / Esc   => Cancel selection and exit
      ?              => Toggle preview pane

    There are many more shortcuts enabled by default, you can also define
    your own shortcuts by setting `--bind` in the $FZF_DEFAULT_OPTS
    environment variable. See `man fzf` for more info.


    Extended Search
    ---------------

    When typing in search terms, the following modifiers can be applied:

      'word: exact match (line must contain the literal string "word")
      ^word: exact prefix match (line must start with literal "word")
      word$: exact suffix match (line must end with literal "word")
      !word: exact negation match (line must not contain literal "word")
      'a | 'b: OR operator (joins two exact match operators together)

    For example:

      ^start 'exact | !ignore fuzzy end$


    Documentation
    -------------

    For more detailed documentation, please see:
    https://firefox-source-docs.mozilla.org/tools/try/selectors/fuzzy.html
    """
    init(command_context)
    if kwargs.pop("interactive"):
        kwargs["query"].append("INTERACTIVE")

    if kwargs.pop("intersection"):
        kwargs["intersect_query"] = kwargs["query"]
        del kwargs["query"]

    if kwargs.get("save") and not kwargs.get("query"):
        # If saving preset without -q/--query, allow user to use the
        # interface to build the query.
        kwargs_copy = kwargs.copy()
        kwargs_copy["dry_run"] = True
        kwargs_copy["save"] = None
        kwargs["query"] = run(command_context, save_query=True, **kwargs_copy)
        if not kwargs["query"]:
            return

    if kwargs.get("tag") and kwargs.get("paths"):
        command_context._mach_context.handler.parser.error(
            "ERROR: only --tag or <path> can be used, not both."
        )
        return

    if kwargs.get("paths"):
        kwargs["test_paths"] = kwargs["paths"]

    if kwargs.get("tag"):
        kwargs["test_tag"] = kwargs["tag"]

    return run(command_context, **kwargs)


@SubCommand(
    "try",
    "chooser",
    description="Schedule tasks by selecting them from a web interface.",
    parser=get_parser("chooser"),
    virtualenv_name="try",
)
def try_chooser(command_context, **kwargs):
    """Push tasks selected from a web interface to try.

    This selector will build the taskgraph and spin up a dynamically
    created 'trychooser-like' web-page on the localhost. After a selection
    has been made, pressing the 'Push' button will automatically push the
    selection to try.
    """
    init(command_context)
    command_context.activate_virtualenv()

    return run(command_context, **kwargs)


@SubCommand(
    "try",
    "auto",
    description="Automatically determine which tasks to run. This runs the same "
    "set of tasks that would be run on autoland. This "
    "selector is EXPERIMENTAL.",
    parser=get_parser("auto"),
    virtualenv_name="try",
)
def try_auto(command_context, **kwargs):
    init(command_context)
    return run(command_context, **kwargs)


@SubCommand(
    "try",
    "again",
    description="Schedule a previously generated push again.",
    parser=get_parser("again"),
    virtualenv_name="try",
)
def try_again(command_context, **kwargs):
    init(command_context)
    return run(command_context, **kwargs)


@SubCommand(
    "try",
    "empty",
    description="Push to try without scheduling any tasks.",
    parser=get_parser("empty"),
    virtualenv_name="try",
)
def try_empty(command_context, **kwargs):
    """Push to try, running no builds or tests

    This selector does not prompt you to run anything, it just pushes
    your patches to try, running no builds or tests by default. After
    the push finishes, you can manually add desired jobs to your push
    via Treeherder's Add New Jobs feature, located in the per-push
    menu.
    """
    init(command_context)
    return run(command_context, **kwargs)


@SubCommand(
    "try",
    "coverage",
    description="Select tasks on try using coverage data",
    parser=get_parser("coverage"),
    virtualenv_name="try",
)
def try_coverage(command_context, **kwargs):
    """Select which tasks to use using coverage data."""
    init(command_context)
    return run(command_context, **kwargs)


@SubCommand(
    "try",
    "release",
    description="Push the current tree to try, configured for a staging release.",
    parser=get_parser("release"),
    virtualenv_name="try",
)
def try_release(command_context, **kwargs):
    """Push the current tree to try, configured for a staging release."""
    init(command_context)
    return run(command_context, **kwargs)


@SubCommand(
    "try",
    "scriptworker",
    description="Run scriptworker tasks against a recent release.",
    parser=get_parser("scriptworker"),
    virtualenv_name="try",
)
def try_scriptworker(command_context, **kwargs):
    """Run scriptworker tasks against a recent release.

    Requires VPN and shipit access.
    """
    init(command_context)
    return run(command_context, **kwargs)


@SubCommand(
    "try",
    "compare",
    description="Push two try jobs, one on your current commit and another on the one you specify",
    parser=get_parser("compare"),
    virtualenv_name="try",
)
def try_compare(command_context, **kwargs):
    init(command_context)
    return run(command_context, **kwargs)


@SubCommand(
    "try",
    "perf",
    description="Try selector for running performance tests.",
    parser=get_parser("perf"),
    virtualenv_name="try",
)
def try_perf(command_context, **kwargs):
    init(command_context)
    return run(command_context, **kwargs)
