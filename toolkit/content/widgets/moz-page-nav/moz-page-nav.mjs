/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import { html, when } from "chrome://global/content/vendor/lit.all.mjs";
import { MozLitElement } from "chrome://global/content/lit-utils.mjs";
// eslint-disable-next-line import/no-unassigned-import
import "chrome://global/content/elements/moz-support-link.mjs";

/**
 * @typedef {("mobile"|"default")} PageNavType
 * @property {PageNavType} [type] - The type of the component
 */

/**
 * A grouping of navigation buttons that is displayed at the page level,
 * intended to change the selected view, provide a heading, and have links
 * to external resources.
 *
 * @tagname moz-page-nav
 * @property {string} currentView - The currently selected view.
 * @property {string} heading - A heading to be displayed at the top of the navigation.
 * @property {PageNavType} [type] - The type of the component
 * @slot [default] - Used to append moz-page-nav-button elements to the navigation.
 * @slot [subheading] - Used to append page specific search input or notification to the nav.
 */
export default class MozPageNav extends MozLitElement {
  static properties = {
    currentView: { type: String },
    heading: { type: String, fluent: true },
    type: { type: String, reflect: true },
  };

  static queries = {
    headingEl: "#page-nav-heading",
    primaryNavGroupSlot: ".primary-nav-group slot",
    secondaryNavGroupSlot: "#secondary-nav-group slot",
  };

  constructor() {
    super();
    /**
     * @type {PageNavType}
     */
    this.type = "default";
  }

  get pageNavButtons() {
    return this.getVisibleSlottedChildren(this.primaryNavGroupSlot);
  }

  get secondaryNavButtons() {
    return this.getVisibleSlottedChildren(this.secondaryNavGroupSlot);
  }

  getVisibleSlottedChildren(el) {
    return el
      ?.assignedElements()
      .filter(
        element =>
          element?.localName === "moz-page-nav-button" &&
          this.checkElementVisibility(element)
      );
  }

  checkElementVisibility(element) {
    let computedStyles = window.getComputedStyle(element);
    return (
      !element.hidden &&
      computedStyles.getPropertyValue("display") !== "none" &&
      computedStyles.getPropertyValue("visibility") !== "hidden" &&
      computedStyles.getPropertyValue("opacity") > 0
    );
  }

  onChangeView(e) {
    this.currentView = e.target.view;
  }

  handleFocus(e) {
    if (e.key == "ArrowDown" || e.key == "ArrowRight") {
      e.preventDefault();
      this.focusNextView();
    } else if (e.key == "ArrowUp" || e.key == "ArrowLeft") {
      e.preventDefault();
      this.focusPreviousView();
    }
  }

  focusPreviousView() {
    let pageNavButtons = this.pageNavButtons;
    let currentIndex = pageNavButtons.findIndex(b => b.selected);
    let prev = pageNavButtons[currentIndex - 1];
    if (prev) {
      prev.activate();
      prev.buttonEl.focus();
    }
  }

  focusNextView() {
    let pageNavButtons = this.pageNavButtons;
    let currentIndex = pageNavButtons.findIndex(b => b.selected);
    let next = pageNavButtons[currentIndex + 1];
    if (next) {
      next.activate();
      next.buttonEl.focus();
    }
  }

  onSecondaryNavChange(event) {
    let secondaryNavElements = event.target.assignedElements();
    this.hasSecondaryNav = !!secondaryNavElements.length;
    secondaryNavElements?.forEach(el => {
      el.classList.add("secondary-nav-item");
    });
  }

  render() {
    return html`
      <link
        rel="stylesheet"
        href="chrome://global/content/elements/moz-page-nav.css"
      />
      <div class="page-nav-heading-wrapper">
        ${this.type === "mobile"
          ? html`<moz-button
              type="icon ghost"
              aria-label="Open Menu"
              aria-expanded="false"
              iconsrc="chrome://browser/skin/menu.svg"
            >
            </moz-button>`
          : ""}
        <div class="logo"></div>
        <h1 class="page-nav-heading" id="page-nav-heading">${this.heading}</h1>
      </div>
      <slot name="subheading"></slot>
      <nav>
        <div
          class="primary-nav-group"
          role="tablist"
          aria-orientation="vertical"
          aria-labelledby="page-nav-heading"
        >
          <slot
            @change-view=${this.onChangeView}
            @keydown=${this.handleFocus}
          ></slot>
        </div>
        ${when(this.hasSecondaryNav, () => html`<hr />`)}
        <div id="secondary-nav-group" role="group">
          <slot
            name="secondary-nav"
            @slotchange=${this.onSecondaryNavChange}
          ></slot>
        </div>
      </nav>
    `;
  }

  updated() {
    let isViewSelected = false;
    let assignedPageNavButtons = this.pageNavButtons;
    for (let button of assignedPageNavButtons) {
      button.selected = button.view == this.currentView;
      isViewSelected = isViewSelected || button.selected;
    }
    if (!isViewSelected && assignedPageNavButtons.length) {
      // Current page nav has no matching view, reset to the first view.
      assignedPageNavButtons[0].activate();
    }
  }
}
customElements.define("moz-page-nav", MozPageNav);

/**
 * A navigation button intended to change the selected view within a page.
 *
 * @tagname moz-page-nav-button
 * @property {string} href - (optional) The url for an external link if not a support page URL
 * @property {string} iconSrc - The chrome:// url for the icon used for the button.
 * @property {boolean} selected - Whether or not the button is currently selected.
 * @property {string} supportPage - (optional) The short name for the support page a secondary link should launch to
 * @slot [default] - Used to append the l10n string to the button.
 */
export class MozPageNavButton extends MozLitElement {
  static properties = {
    iconSrc: { type: String, reflect: true },
    href: { type: String },
    selected: { type: Boolean },
    supportPage: { type: String, attribute: "support-page" },
  };

  connectedCallback() {
    super.connectedCallback();
    this.setAttribute("role", "none");
  }

  static queries = {
    buttonEl: "button",
    linkEl: "a",
  };

  get view() {
    return this.getAttribute("view");
  }

  activate() {
    this.dispatchEvent(
      new CustomEvent("change-view", {
        bubbles: true,
        composed: true,
      })
    );
  }

  itemTemplate() {
    if (this.href || this.supportPage) {
      return this.linkTemplate();
    }
    return this.buttonTemplate();
  }

  buttonTemplate() {
    return html`
      <button
        aria-selected=${this.selected}
        tabindex=${this.selected ? 0 : -1}
        role="tab"
        ?selected=${this.selected}
        @click=${this.activate}
      >
        ${this.innerContentTemplate()}
      </button>
    `;
  }

  linkTemplate() {
    if (this.supportPage) {
      return html`
        <a
          is="moz-support-link"
          class="moz-page-nav-link"
          support-page=${this.supportPage}
        >
          ${this.innerContentTemplate()}
        </a>
      `;
    }
    return html`
      <a href=${this.href} class="moz-page-nav-link" target="_blank">
        ${this.innerContentTemplate()}
      </a>
    `;
  }

  innerContentTemplate() {
    return html`
      ${this.iconSrc
        ? html`<img
            class="page-nav-icon"
            src=${this.iconSrc}
            role="presentation"
          />`
        : ""}
      <slot></slot>
    `;
  }

  render() {
    return html`
      <link
        rel="stylesheet"
        href="chrome://global/content/elements/moz-page-nav-button.css"
      />
      ${this.itemTemplate()}
    `;
  }
}
customElements.define("moz-page-nav-button", MozPageNavButton);
