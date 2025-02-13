#pragma once


void SimulateModalDialog(CDialog* dialog, std::function<void(CWnd* parent_window)> create_dialog_callback);
