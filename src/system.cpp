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
FILENAME: src/system.cpp
DESCRIPTION: System information for DieKnow
AUTHOR: Ethan Chan
DATE: 2024-11-13
VERSION: 1.0.1
*/

#include <windows.h>
#include <string>
#include <sstream>
#include <chrono>
#include <thread>


const double WINDOW_DELAY = 0.7;


std::string get_cpu_name() {
    HKEY hkey;
    char cpu_name[256];
    DWORD buffer_size = sizeof(cpu_name);

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                      "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                      0, KEY_READ, &hkey) == ERROR_SUCCESS) {
        RegQueryValueExA(
            hkey,
            "ProcessorNameString",
            NULL, NULL,
            (LPBYTE)cpu_name,
            &buffer_size);

        RegCloseKey(hkey);
    }
    return std::string(cpu_name);
}

std::string get_gpu_name() {
    DISPLAY_DEVICEA dd;
    dd.cb = sizeof(dd);
    std::string name = "Unknown GPU";

    if (EnumDisplayDevicesA(NULL, 0, &dd, 0)) {
        name = dd.DeviceString;
    }
    return name;
}

std::string get_os_info() {
    SYSTEM_INFO si;
    GetNativeSystemInfo(&si);
    std::string arch = (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) ? "64-bit" : "32-bit";

    OSVERSIONINFOEXA osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEXA));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((LPOSVERSIONINFOA)&osvi);

    std::ostringstream os_info;
    os_info << "Windows " << osvi.dwMajorVersion << "." << osvi.dwMinorVersion
           << " (Build " << osvi.dwBuildNumber << "), " << arch;
    return os_info.str();
}

std::string get_available_ram() {
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);

    std::ostringstream ram_info;
    ram_info << (statex.ullAvailPhys / (1024 * 1024)) << " MB available";
    return ram_info.str();
}

void press(BYTE key) {
    keybd_event(key, 0, 0, 0);
}

void release(BYTE key) {
    keybd_event(key, 0, KEYEVENTF_KEYUP, 0);
}

inline void push(BYTE key) {
    press(key);
    release(key);
}

void toggle_internet() {
    /*
    Toggle internet by the following keypresses:
    
    1. Press Windows-A
    2. Press Space
    3. Press Windows-A
    4. Press Escape to close the window
    */

    press(0x5B);
    press(0x41);
    release(0x41);
    release(0x5b);

    std::this_thread::sleep_for(std::chrono::duration<double>(WINDOW_DELAY));

    push(0x20);

    push(0x1B);
}
