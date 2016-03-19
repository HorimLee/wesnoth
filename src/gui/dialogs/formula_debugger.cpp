/*
   Copyright (C) 2009 - 2016 by Yurii Chernyi <terraninfo@terraninfo.net>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#define GETTEXT_DOMAIN "wesnoth-lib"

#include "gui/dialogs/formula_debugger.hpp"

#include "gui/auxiliary/find_widget.tpp"
#include "gui/dialogs/helper.hpp"
#include "gui/widgets/button.hpp"
#include "gui/widgets/settings.hpp"
#include "gui/widgets/window.hpp"
#include "../../formula_debugger.hpp" // We want the file in src/
#include "utils/foreach.tpp"

#include <boost/bind.hpp>

namespace {

std::string pango_escape(std::string str) {
	for(size_t i = str.size(); i > 0; i--) {
		if(str[i-1] == '<') {
			str.replace(i-1, 1, "&lt;");
		} else if(str[i-1] == '>') {
			str.replace(i-1, 1, "&gt;");
		} else if(str[i-1] == '&') {
			str.replace(i-1, 1, "&amp;");
		} else if(str[i-1] == '"') {
			str.replace(i-1, 1, "&quot;");
		} else if(str[i-1] == '\'') {
			str.replace(i-1, 1, "&apos;");
		}
	}
	return str;
}

}

namespace gui2
{

/*WIKI
 * @page = GUIWindowDefinitionWML
 * @order = 2_formula_debugger
 *
 * == Formula debugger ==
 *
 * This shows the debugger for the formulas.
 *
 * @begin{table}{dialog_widgets}
 *
 * stack & & control & m &
 *         A stack. $
 *
 * execution & & control & m &
 *         Execution trace label. $
 *
 * state & & control & m &
 *         The state. $
 *
 * step & & button & m &
 *         Button to step into the execution. $
 *
 * stepout & & button & m &
 *         Button to step out of the execution. $
 *
 * next & & button & m &
 *         Button to execute the next statement. $
 *
 * continue & & button & m &
 *         Button to continue the execution. $
 *
 * @end{table}
 */

REGISTER_DIALOG(formula_debugger)

void tformula_debugger::pre_show(twindow& window)
{
	// stack label
	tcontrol* stack_label
			= find_widget<tcontrol>(&window, "stack", false, true);

	std::stringstream stack_text;
	std::string indent = "  ";
	int c = 0;
	FOREACH(const AUTO & i, fdb_.get_call_stack())
	{
		for(int d = 0; d < c; ++d) {
			stack_text << indent;
		}
		stack_text << "#<span color=\"green\">" << i.counter()
				   << "</span>: \"<span color=\"green\">" << pango_escape(i.name())
				   << "</span>\": (" << pango_escape(i.str()) << ") " << std::endl;
		++c;
	}

	stack_label->set_use_markup(true);
	stack_label->set_label(stack_text.str());
	window.keyboard_capture(stack_label);

	// execution trace label
	tcontrol* execution_label
			= find_widget<tcontrol>(&window, "execution", false, true);

	std::stringstream execution_text;
	FOREACH(const AUTO & i, fdb_.get_execution_trace())
	{
		for(int d = 0; d < i.level(); ++d) {
			execution_text << indent;
		}
		if(!i.evaluated()) {
			execution_text << "#<span color=\"green\">" << i.counter()
						   << "</span>: \"<span color=\"green\">" << pango_escape(i.name())
						   << "</span>\": (" << pango_escape(i.str()) << ") " << std::endl;
		} else {
			execution_text << "#<span color=\"yellow\">" << i.counter()
						   << "</span>: \"<span color=\"yellow\">" << pango_escape(i.name())
						   << "</span>\": (" << pango_escape(i.str()) << ") = "
						   << "<span color=\"orange\">"
						   << pango_escape(i.value().to_debug_string(NULL, false))
						   << "</span>" << std::endl;
		}
	}

	execution_label->set_use_markup(true);
	execution_label->set_label(execution_text.str());

	// state
	std::string state_str;
	bool is_end = false;
	if(!fdb_.get_current_breakpoint()) {
		state_str = "";
	} else {
		state_str = fdb_.get_current_breakpoint()->name();
		if(state_str == "End") {
			is_end = true;
		}
	}

	find_widget<tcontrol>(&window, "state", false).set_label(state_str);

	// callbacks
	tbutton& step_button = find_widget<tbutton>(&window, "step", false);
	connect_signal_mouse_left_click(
			step_button,
			boost::bind(&tformula_debugger::callback_step_button,
						this,
						boost::ref(window)));

	tbutton& stepout_button = find_widget<tbutton>(&window, "stepout", false);
	connect_signal_mouse_left_click(
			stepout_button,
			boost::bind(&tformula_debugger::callback_stepout_button,
						this,
						boost::ref(window)));

	tbutton& next_button = find_widget<tbutton>(&window, "next", false);
	connect_signal_mouse_left_click(
			next_button,
			boost::bind(&tformula_debugger::callback_next_button,
						this,
						boost::ref(window)));

	tbutton& continue_button = find_widget<tbutton>(&window, "continue", false);
	connect_signal_mouse_left_click(
			continue_button,
			boost::bind(&tformula_debugger::callback_continue_button,
						this,
						boost::ref(window)));

	if(is_end) {
		step_button.set_active(false);
		stepout_button.set_active(false);
		next_button.set_active(false);
		continue_button.set_active(false);
	}
}

void tformula_debugger::callback_continue_button(twindow& window)
{
	fdb_.add_breakpoint_continue_to_end();
	window.set_retval(twindow::OK);
}

void tformula_debugger::callback_next_button(twindow& window)
{
	fdb_.add_breakpoint_next();
	window.set_retval(twindow::OK);
}

void tformula_debugger::callback_step_button(twindow& window)
{
	fdb_.add_breakpoint_step_into();
	window.set_retval(twindow::OK);
}

void tformula_debugger::callback_stepout_button(twindow& window)
{
	fdb_.add_breakpoint_step_out();
	window.set_retval(twindow::OK);
}

} // end of namespace gui2
