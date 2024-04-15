This folder contains code that attempts to run SC4's built-in MessageBox using the cIGZUIScriptService.

The dialog runs, but for some reason the game's modal dialog message loop does not appear to receive the
command that is sent to close the dialog and exit the message loop when the OK button is clicked.
I have no idea what the issue is, as the code calls the same method in the window manager that all
of the built-in dialog use.