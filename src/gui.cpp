/*
COPYRIGHT (C) 2024 ETHAN CHAN

ALL RIGHTS RESERVED. UNAUTHORIZED COPYING, MODIFICATION, DISTRIBUTION, OR USE
OF THIS SOFTWARE WITHOUT PRIOR PERMISSION IS STRICTLY PROHIBITED.

THIS SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES, OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT, OR OTHERWISE, ARISING FROM,
OUT OF, OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

PROJECT NAME: DieKnow
FILENAME: src/gui.cpp
DESCRIPTION: GUI interface to DieKnow API
AUTHOR: Ethan Chan
DATE: 2024-11-13
VERSION: 1.0.1
*/

#include <vector>
#include <string>
#include <windows.h>
#include <commctrl.h>

#include "api.cpp"
#include "system.cpp"

// Or more correctly, widget dimensions
const int BUTTON_WIDTH = 200;
const int BUTTON_HEIGHT = 35;

// Space between widgets as padding
const int PADDING = 10;

namespace Widgets {
    enum Button {
        RUNNING = 0,
        TASKKILL,
        EXIT,
        DIRECTORY,
        INTERVAL_LABEL,
        INTERVAL,
        INTERVAL_SET,
        EXECUTABLES_KILLED,
        WINDOW_SHOWER,
        WINDOWS,
        OPEN_EXPLORER,
        SYSTEM_INFORMATION
    };
}


extern "C" {
    __declspec(dllexport) void create_window();
}

void tooltip(HWND hwnd, HWND control, const char* text) {
    /*
    Display a tooltip to aid user interactions.
    */

    HWND htooltip = CreateWindowEx(
        0, TOOLTIPS_CLASS, NULL,
        WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        hwnd, NULL, NULL, NULL
    );

    TOOLINFO tool_info = {};
    tool_info.cbSize = sizeof(tool_info);
    tool_info.uFlags = TTF_SUBCLASS;
    tool_info.hwnd = control;
    tool_info.hinst = NULL;
    tool_info.lpszText = const_cast<LPSTR>(text);

    // Get dimensions of the control
    GetClientRect(control, &tool_info.rect);
    SendMessage(htooltip, TTM_ADDTOOL, 0, (LPARAM)&tool_info);
}

void write(const std::string& filename, int value) {
    /*
    Write an integer to a file.
    */

    std::ofstream file(filename);

    if (file.is_open()) {
        file << value;
        file.close();
    } else {
        std::ostringstream message;
        message << "Unable to open the file "
                << filename << ".\n\n"
                << "Ensure it:" << "\n"
                << "* Exists," << "\n"
                << "* Is not in use by another application, and" << "\n"
                << "* Is avaliable and downloaded to OneDrive." << "\n";
            
        MessageBox(nullptr, message.str().c_str(), "Error", MB_ICONERROR);
    }
}

int read(const std::string& filename) {
    /*
    Read an integer from a file.
    */

    std::ifstream file(filename);
    int value = 0;

    if (file.is_open()) {
        file >> value;
        file.close();
    } else {
        MessageBox(nullptr, "Failed to open file", "Error", MB_ICONERROR);
    }

    return value;
}

const char* get_selected(HWND listbox) {
    /*
    Get the selected item in a listbox.
    */

    int index = SendMessage(listbox, LB_GETCURSEL, 0, 0);
    if (index == LB_ERR) {
        // Unable to get listbox contents for some reason (no selection?)
        return "";
    }

    // Needed to specify memory allocation
    int length = SendMessage(listbox, LB_GETTEXTLEN, index, 0);

    // Create a character buffer to output the selected item
    char* buffer = new char[length + 1];
    SendMessage(listbox, LB_GETTEXT, index, (LPARAM)buffer);

    const char* text = _strdup(buffer);

    // Cleanup
    delete[] buffer;

    return text;
}

class Application {
public:
    // Used to call `WM_SETFONT`
    std::vector<HWND> widgets;

    // Used to determine whether or not to refresh the listbox
    std::vector<std::string> previous_executables;

    bool is_ws_registered = false;

    Application() {
        validate();

        // Used for help popup balloon
        InitCommonControls();

        const char CLASS_NAME[] = "DieKnow";

        WNDCLASS wc = {};
        wc.lpfnWndProc = Application::WindowProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszClassName = CLASS_NAME;

        RegisterClass(&wc);

        HFONT main_font = CreateFont(
            18,
            0,
            0,
            0,
            FW_NORMAL,
            FALSE,
            FALSE,
            FALSE,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_SWISS,
            "Segoe UI");

        HWND hwnd = CreateWindowEx(
            0,
            CLASS_NAME,
            "DieKnow",
            WS_OVERLAPPEDWINDOW & ~(WS_MAXIMIZEBOX),
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            NULL, NULL, wc.hInstance, NULL);

        if (hwnd == NULL) {
            return;
        }

        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

        // Resize the window
        MoveWindow(hwnd, 0, 0, (BUTTON_WIDTH * 2) + (10 * 5), 600, TRUE);

        HWND running_button = CreateWindow(
            "BUTTON",
            "Start",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            PADDING,
            PADDING,
            BUTTON_WIDTH,
            BUTTON_HEIGHT,
            hwnd,
            (HMENU)Widgets::RUNNING,
            wc.hInstance,
            NULL);

        HWND taskkill_button = CreateWindow(
            "BUTTON",
            "Terminate selected",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            PADDING,
            BUTTON_HEIGHT + (PADDING * 2),
            BUTTON_WIDTH,
            BUTTON_HEIGHT,
            hwnd,
            (HMENU)Widgets::TASKKILL,
            wc.hInstance,
            NULL);

        HWND exit_button = CreateWindow(
            "BUTTON",
            "Quit and Exit",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            PADDING,
            (BUTTON_HEIGHT * 2) + (PADDING * 3),
            BUTTON_WIDTH,
            BUTTON_HEIGHT,
            hwnd,
            (HMENU)Widgets::EXIT,
            wc.hInstance,
            NULL
        );
        HWND directory = CreateWindow(
            "LISTBOX",
            nullptr,
            WS_VISIBLE | WS_CHILD | LBS_STANDARD,
            BUTTON_WIDTH + (PADDING * 2),
            PADDING,
            BUTTON_WIDTH,
            170,
            hwnd,
            (HMENU)Widgets::DIRECTORY,
            wc.hInstance,
            NULL
        );
        HWND interval_label = CreateWindow(
            "STATIC",
            "Interval:",
            WS_VISIBLE | WS_CHILD,
            PADDING,
            158 + BUTTON_HEIGHT,
            50, 18,
            hwnd,
            (HMENU)Widgets::INTERVAL_LABEL,
            wc.hInstance,
            NULL
        );
        HWND interval_edit = CreateWindow(
            "EDIT",
            "",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
            50 + (PADDING * 2),
            156 + BUTTON_HEIGHT,
            50, 22,
            hwnd,
            (HMENU)Widgets::INTERVAL,
            wc.hInstance,
            NULL
        );
        HWND interval_set = CreateWindow(
            "BUTTON",
            "Set interval",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            BUTTON_WIDTH + (PADDING * 2),
            150 + BUTTON_HEIGHT,
            BUTTON_WIDTH,
            BUTTON_HEIGHT,
            hwnd,
            (HMENU)Widgets::INTERVAL_SET,
            wc.hInstance,
            NULL
        );
        HWND executables_killed = CreateWindow(
            "STATIC",
            "Executables terminated:",
            WS_VISIBLE | WS_CHILD,
            PADDING,
            150 + (BUTTON_HEIGHT * 2) + PADDING,
            BUTTON_WIDTH, 18,
            hwnd,
            (HMENU)Widgets::EXECUTABLES_KILLED,
            wc.hInstance,
            NULL
        );
        HWND window_shower = CreateWindow(
            "BUTTON",
            "Window shower...",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            PADDING,
            150 + (BUTTON_HEIGHT * 3) + (PADDING * 2),
            BUTTON_WIDTH,
            BUTTON_HEIGHT,
            hwnd,
            (HMENU)Widgets::WINDOW_SHOWER,
            wc.hInstance,
            NULL
        );
        HWND open_explorer = CreateWindow(
            "BUTTON",
            "Open in Explorer",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            BUTTON_WIDTH + (PADDING * 2),
            150 + (BUTTON_HEIGHT * 2) + PADDING,
            BUTTON_WIDTH,
            BUTTON_HEIGHT,
            hwnd,
            (HMENU)Widgets::OPEN_EXPLORER,
            wc.hInstance,
            NULL
        );
        HWND display_information = CreateWindow(
            "BUTTON",
            "System information...",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            BUTTON_WIDTH + (PADDING * 2),
            150 + (BUTTON_HEIGHT * 3) + (PADDING * 2),
            BUTTON_WIDTH,
            BUTTON_HEIGHT,
            hwnd,
            (HMENU)Widgets::SYSTEM_INFORMATION,
            wc.hInstance,
            NULL
        );

        widgets.push_back(running_button);
        widgets.push_back(taskkill_button);
        widgets.push_back(exit_button);
        widgets.push_back(directory);
        widgets.push_back(interval_label);
        widgets.push_back(interval_edit);
        widgets.push_back(interval_set);
        widgets.push_back(executables_killed);
        widgets.push_back(window_shower);
        widgets.push_back(open_explorer);
        widgets.push_back(display_information);

        tooltip(hwnd, running_button, "Toggle between DieKnow running or stopped.");
        tooltip(hwnd, taskkill_button, "Terminate the selected executable in the listbox.");
        tooltip(hwnd, exit_button, "Exit the DieKnow application and terminate all processes.");
        tooltip(hwnd, directory, "Directory of the DyKnow files.");
        tooltip(hwnd, interval_edit, "Delay between ticks for closing DyKnow.");
        tooltip(hwnd, interval_set, "Set the interval between ticks for closing DyKnow. Beware - an interval of 0 can saturate a CPU core.");
        tooltip(hwnd, executables_killed, "Number of DyKnow executables terminated by DieKnow.");
        tooltip(hwnd, open_explorer, "Open the DyKnow file directory in the Windows Explorer.");
        tooltip(hwnd, display_information, "Show system information.");

        for (HWND widget : widgets) {
            SendMessage(widget, WM_SETFONT, (WPARAM)main_font, TRUE);
        }

        // In ms -- set to 5 ticks per second
        SetTimer(hwnd, 1, 200, nullptr);

        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);

        MSG msg = {};
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    void manage_command(Application* app, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        /*
        Manage button commands in a switch statement.

        This function is called by `WindowProc`.
        */

        switch (LOWORD(wParam)) {
            case Widgets::RUNNING: {
                if (running) {
                    toggle_internet();
                    stop_monitoring();
                    toggle_internet();
                } else {
                    SetFocus(NULL);
                    toggle_internet();
                    start_monitoring(FOLDER_PATH);
                    toggle_internet();
                }

                std::string status = running ? "Stop" : "Start";
                SetWindowText(app->widgets[Widgets::RUNNING], status.c_str());
                break;
            }

            case Widgets::TASKKILL: {
                // Check if the listbox has a selected item
                const char* selected = get_selected(app->widgets[Widgets::DIRECTORY]);

                // If it does, terminate its process
                if (selected && strlen(selected) > 0) {
                    close_application_by_exe(selected);

                    std::string message = "Successfully closed " + std::string(selected);
                    MessageBox(hwnd, message.c_str(), "Success", MB_ICONINFORMATION);
                }
                else {
                    // Display an error if it doesn't
                    MessageBox(hwnd, "Please select an item in the listbox.", "Error", MB_ICONERROR);
                }
                break;
            }

            case Widgets::INTERVAL_SET: {
                char buffer[16];

                GetWindowText(app->widgets[Widgets::INTERVAL], buffer, sizeof(buffer));

                int value = atoi(buffer);

                if (value > 0) {
                    write("../interval.txt", value);

                    std::string message = "Successfully set interval buffer to " + std::string(buffer);

                    MessageBox(hwnd, message.c_str(), "Message", MB_ICONINFORMATION);
                }
                break;
            }

            case Widgets::WINDOW_SHOWER: {
                const char* ws_class_name = "WindowShower";

                if (!app->is_ws_registered) {
                    WNDCLASSS ws_wc = {};
                    ws_wc.lpfnWndProc = Application::WSWindowProc;
                    ws_wc.hInstance = GetModuleHandle(NULL);
                    ws_wc.lpszClassName = ws_class_name;

                    if (!RegisterClass(&ws_wc)) {
                        MessageBox(NULL, "Window class registration for window shower failed!", "Error", MB_ICONERROR);
                        return;
                    }

                    app->is_ws_registered = true;
                }

                HWND ws_hwnd = CreateWindowEx(
                    0,
                    ws_class_name,
                    "Window Shower",
                    WS_OVERLAPPEDWINDOW,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    500,
                    400,
                    NULL,
                    NULL,
                    ws_wc.hInstance,
                    NULL
                );

                if (ws_hwnd == NULL) {
                    MessageBox(NULL, "Window creation failed for new window!", "Error", MB_OK);
                    return;
                }

                HWND listbox = CreateWindow(
                    "LISTBOX",
                    nullptr,
                    WS_VISIBLE | WS_CHILD | LBS_STANDARD,
                    PADDING,
                    PADDING,
                    300,
                    400,
                    hwnd,
                    (HMENU)Widgets::WINDOWS,
                    ws_wc.hInstance,
                    NULL
                );

                ShowWindow(ws_hwnd, SW_SHOWNORMAL);
                UpdateWindow(ws_hwnd);

                break;
            }

            case Widgets::OPEN_EXPLORER: {
                ShellExecute(NULL, "open", FOLDER_PATH, NULL, NULL, SW_SHOWDEFAULT);
                break;
            }

            case Widgets::SYSTEM_INFORMATION: {
                std::string cpu_name = get_cpu_name();
                std::string gpu_name = get_gpu_name();
                std::string os_info = get_os_info();
                std::string avaliable_ram = get_available_ram();

                std::ostringstream message;
                message << "SYSTEM INFORMATION" << "\n"
                        << "==================" << "\n"
                        << "CPU: " << cpu_name << "\n"
                        << "GPU: " << gpu_name << "\n"
                        << "Operating system: " << os_info << "\n"
                        << "Free RAM: " << avaliable_ram;

                MessageBox(hwnd, message.str().c_str(), "System Information", MB_OK | MB_ICONINFORMATION);

                break;
            }

            case Widgets::EXIT: {
                DestroyWindow(hwnd);
                break;
            }
        }
    }

    // Use `static` to allow it to be called as an event by Windows API
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        /*
        Manage window events.

        This is called internally by the Windows API.
        */

        // We'll have to use reinterpret_cast as this function is static
        Application* app = reinterpret_cast<Application*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

        switch (uMsg) {
            case WM_COMMAND:
                app->manage_command(app, hwnd, uMsg, wParam, lParam);
                break;

            case WM_CHAR:
                if (GetFocus() == app->widgets[Widgets::INTERVAL]) {
                    if (wParam == VK_RETURN) {
                        SetFocus(NULL);
                        return 0;
                    }
                }

                break;

            case WM_TIMER:
                if (wParam == 1) {
                    app->update();
                }
                return 0;

            case WM_DESTROY:
                KillTimer(hwnd, 1);
                PostQuitMessage(0);
                return 0;
        }

        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    static LRESULT CALLBACK WSWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        /*
        WindowProc for window shower.
        */

        switch (uMsg) {
            case WM_DESTROY:
                DestroyWindow(0);
                return 0;
                break;

            default:
                return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
    }

    void update() {
        /*
        Update display labels and listbox.

        * Update listbox if files have been changed
        * Update interval text based on `interval.txt`
        * Update label displaying executables terminated
        */

        // Update directory listbox
        std::vector<std::string> current_executables;

        for (const auto& entry : fs::directory_iterator(FOLDER_PATH)) {
            if (entry.is_regular_file() && entry.path().extension() == ".exe") {
                current_executables.push_back(entry.path().filename().string());
            }
        }

        if (current_executables == previous_executables) {
            return;
        }

        previous_executables = current_executables;

        SendMessage(widgets[Widgets::DIRECTORY], LB_RESETCONTENT, 0, 0);

        for (const std::string& file_name : current_executables) {
            SendMessage(
                widgets[Widgets::DIRECTORY],
                LB_ADDSTRING, 0,
                (LPARAM)file_name.c_str());
        }

        // Update window shower listbox

        std::vector<Window> windows;

        SendMessage(widgets[Widgets::WINDOWS], LB_RESETCONTENT, 0, 0);

        EnumWindows(enum_windows, reinterpret_cast<LPARAM>(&windows));

        for (const Window& window : windows) {
            SendMessage(
                widgets[Widgets::WINDOWS],
                LB_ADDSTRING, 0,
                (LPARAM)window.c_str()
            );
        }

        if (GetFocus() != widgets[Widgets::INTERVAL]) {
            SetWindowText(
                widgets[Widgets::INTERVAL],
                std::to_string(read("../interval.txt")).c_str());
        }

        std::string message = "Executables terminated: " + std::to_string(get_killed_count());
        SetWindowText(
            widgets[Widgets::EXECUTABLES_KILLED],
            message.c_str());
    }
};

void create_window() {
    Application* application = new Application();
    delete application;
}
