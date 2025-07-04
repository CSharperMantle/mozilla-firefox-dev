# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

import sys

import ReservedWordReader


def line(opt, s):
    opt["output"].write("{}{}\n".format("    " * opt["indent_level"], s))


def indent(opt):
    opt["indent_level"] += 1


def dedent(opt):
    opt["indent_level"] -= 1


def span_and_count_at(reserved_word_list, column):
    assert len(reserved_word_list) != 0

    chars_dict = {}
    for index, word in reserved_word_list:
        chars_dict[ord(word[column])] = True

    chars = sorted(chars_dict.keys())
    return chars[-1] - chars[0] + 1, len(chars)


def optimal_switch_column(opt, reserved_word_list, columns, unprocessed_columns):
    assert len(reserved_word_list) != 0
    assert unprocessed_columns != 0

    min_count = 0
    min_span = 0
    min_count_index = 0
    min_span_index = 0

    for index in range(0, unprocessed_columns):
        span, count = span_and_count_at(reserved_word_list, columns[index])
        assert span != 0

        if span == 1:
            assert count == 1
            return 1, True

        assert count != 1
        if index == 0 or min_span > span:
            min_span = span
            min_span_index = index

        if index == 0 or min_count > count:
            min_count = count
            min_count_index = index

    if min_count <= opt["use_if_threshold"]:
        return min_count_index, True

    return min_span_index, False


def split_list_per_column(reserved_word_list, column):
    assert len(reserved_word_list) != 0

    column_dict = {}
    for item in reserved_word_list:
        index, word = item
        per_column = column_dict.setdefault(word[column], [])
        per_column.append(item)

    return sorted(column_dict.items())


def generate_letter_switch(opt, unprocessed_columns, reserved_word_list, columns=None):
    assert len(reserved_word_list) != 0

    if not columns:
        columns = range(0, unprocessed_columns)

    if len(reserved_word_list) == 1:
        index, word = reserved_word_list[0]

        if unprocessed_columns == 0:
            line(opt, f"JSRW_GOT_MATCH({index}) /* {word} */")
            return

        if unprocessed_columns > opt["char_tail_test_threshold"]:
            line(opt, f"JSRW_TEST_GUESS({index}) /* {word} */")
            return

        conds = []
        for column in columns[0:unprocessed_columns]:
            quoted = repr(word[column])
            conds.append(f"JSRW_AT({column})=={quoted}")

        line(opt, "if ({}) {{".format(" && ".join(conds)))

        indent(opt)
        line(opt, f"JSRW_GOT_MATCH({index}) /* {word} */")
        dedent(opt)

        line(opt, "}")
        line(opt, "JSRW_NO_MATCH()")
        return

    assert unprocessed_columns != 0

    optimal_column_index, use_if = optimal_switch_column(
        opt, reserved_word_list, columns, unprocessed_columns
    )
    optimal_column = columns[optimal_column_index]

    # Make a copy to avoid breaking passed list.
    columns = list(columns)
    columns[optimal_column_index] = columns[unprocessed_columns - 1]

    list_per_column = split_list_per_column(reserved_word_list, optimal_column)

    if not use_if:
        line(opt, f"switch (JSRW_AT({optimal_column})) {{")

    for char, reserved_word_list_per_column in list_per_column:
        quoted = repr(char)
        if use_if:
            line(opt, f"if (JSRW_AT({optimal_column}) == {quoted}) {{")
        else:
            line(opt, f"  case {quoted}:")

        indent(opt)
        generate_letter_switch(
            opt, unprocessed_columns - 1, reserved_word_list_per_column, columns
        )
        dedent(opt)

        if use_if:
            line(opt, "}")

    if not use_if:
        line(opt, "}")

    line(opt, "JSRW_NO_MATCH()")


def split_list_per_length(reserved_word_list):
    assert len(reserved_word_list) != 0

    length_dict = {}
    for item in reserved_word_list:
        index, word = item
        per_length = length_dict.setdefault(len(word), [])
        per_length.append(item)

    return sorted(length_dict.items())


def generate_switch(opt, reserved_word_list):
    assert len(reserved_word_list) != 0

    line(opt, "/*")
    line(
        opt,
        f" * Generating switch for the list of {len(reserved_word_list)} entries:",
    )
    for index, word in reserved_word_list:
        line(opt, f" * {word}")
    line(opt, " */")

    list_per_length = split_list_per_length(reserved_word_list)

    use_if = False
    if len(list_per_length) < opt["use_if_threshold"]:
        use_if = True

    if not use_if:
        line(opt, "switch (JSRW_LENGTH()) {")

    for length, reserved_word_list_per_length in list_per_length:
        if use_if:
            line(opt, f"if (JSRW_LENGTH() == {length}) {{")
        else:
            line(opt, f"  case {length}:")

        indent(opt)
        generate_letter_switch(opt, length, reserved_word_list_per_length)
        dedent(opt)

        if use_if:
            line(opt, "}")

    if not use_if:
        line(opt, "}")
    line(opt, "JSRW_NO_MATCH()")


def main(output, reserved_words_h, *args):
    reserved_word_list = ReservedWordReader.read_reserved_word_list(
        reserved_words_h, *args
    )

    opt = {
        "indent_level": 1,
        "use_if_threshold": 3,
        "char_tail_test_threshold": 4,
        "output": output,
    }
    generate_switch(opt, reserved_word_list)


if __name__ == "__main__":
    main(sys.stdout, *sys.argv[1:])
