/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.mozilla.fenix.home.toolbar

import android.content.Context
import androidx.core.content.ContextCompat
import androidx.navigation.NavController
import mozilla.components.browser.state.selector.normalTabs
import mozilla.components.browser.state.selector.privateTabs
import mozilla.components.browser.state.state.BrowserState
import mozilla.components.ui.tabcounter.TabCounterMenu
import mozilla.components.ui.tabcounter.TabCounterView
import mozilla.telemetry.glean.private.NoExtras
import org.mozilla.fenix.GleanMetrics.StartOnHome
import org.mozilla.fenix.NavGraphDirections
import org.mozilla.fenix.R
import org.mozilla.fenix.browser.browsingmode.BrowsingMode
import org.mozilla.fenix.browser.browsingmode.BrowsingModeManager
import org.mozilla.fenix.components.toolbar.FenixTabCounterMenu
import org.mozilla.fenix.ext.nav
import org.mozilla.fenix.tabstray.DefaultTabManagementFeatureHelper
import org.mozilla.fenix.tabstray.Page
import org.mozilla.fenix.tabstray.TabManagementFeatureHelper

/**
 * Helper class for building the [FenixTabCounterMenu].
 *
 * @param context An Android [Context].
 * @param browsingModeManager [BrowsingModeManager] used for fetching the current browsing mode.
 * @param navController [NavController] used for navigation.
 * @param tabCounter The [TabCounterView] that will be setup with event handlers.
 * @param showLongPressMenu Whether a popup menu should be shown when long pressing on this or not.
 * @param tabManagementFeatureHelper Feature flag helper for the tab management UI.
 */
class TabCounterView(
    private val context: Context,
    private val browsingModeManager: BrowsingModeManager,
    private val navController: NavController,
    private val tabCounter: TabCounterView,
    private val showLongPressMenu: Boolean,
    private val tabManagementFeatureHelper: TabManagementFeatureHelper = DefaultTabManagementFeatureHelper,
) {

    init {
        setupLongPressMenu()

        tabCounter.setOnClickListener {
            StartOnHome.openTabsTray.record(NoExtras())

            if (tabManagementFeatureHelper.enhancementsEnabled) {
                navController.nav(
                    navController.currentDestination?.id,
                    NavGraphDirections.actionGlobalTabManagementFragment(
                        page = when (browsingModeManager.mode) {
                            BrowsingMode.Normal -> Page.NormalTabs
                            BrowsingMode.Private -> Page.PrivateTabs
                        },
                    ),
                )
            } else {
                navController.nav(
                    navController.currentDestination?.id,
                    NavGraphDirections.actionGlobalTabsTrayFragment(
                        page = when (browsingModeManager.mode) {
                            BrowsingMode.Normal -> Page.NormalTabs
                            BrowsingMode.Private -> Page.PrivateTabs
                        },
                    ),
                )
            }
        }
    }

    /**
     * Updates the tab counter count based on the current browser state.
     *
     * @param browserState [BrowserState] used to get the current tab count for the current
     * browsing mode.
     */
    fun update(browserState: BrowserState) {
        val isPrivate = browsingModeManager.mode.isPrivate
        val tabCount = if (isPrivate) {
            browserState.privateTabs.size
        } else {
            browserState.normalTabs.size
        }

        tabCounter.setCountWithAnimation(tabCount)
        tabCounter.toggleCounterMask(isPrivate)
        tabCounter.updateContentDescription(isPrivate)
    }

    /**
     * Callback invoked when a menu item is tapped on.
     */
    internal fun onItemTapped(item: TabCounterMenu.Item) {
        if (item is TabCounterMenu.Item.NewTab) {
            browsingModeManager.mode = BrowsingMode.Normal
            val directions =
                NavGraphDirections.actionGlobalSearchDialog(
                    sessionId = null,
                )
            navController.nav(
                navController.currentDestination?.id,
                directions,
            )
        } else if (item is TabCounterMenu.Item.NewPrivateTab) {
            browsingModeManager.mode = BrowsingMode.Private
            val directions =
                NavGraphDirections.actionGlobalSearchDialog(
                    sessionId = null,
                )
            navController.nav(
                navController.currentDestination?.id,
                directions,
            )
        }
    }

    private fun setupLongPressMenu() {
        if (showLongPressMenu) {
            val tabCounterMenu = FenixTabCounterMenu(
                context = context,
                onItemTapped = ::onItemTapped,
                iconColor = if (browsingModeManager.mode == BrowsingMode.Private) {
                    ContextCompat.getColor(context, R.color.fx_mobile_private_icon_color_primary)
                } else {
                    null
                },
            )

            tabCounterMenu.updateMenu(
                showOnly = when (browsingModeManager.mode) {
                    BrowsingMode.Normal -> BrowsingMode.Private
                    BrowsingMode.Private -> BrowsingMode.Normal
                },
            )

            tabCounter.setOnLongClickListener {
                StartOnHome.longClickTabsTray.record(NoExtras())
                tabCounterMenu.menuController.show(anchor = it)
                true
            }
        }
    }
}
