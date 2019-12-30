#define ISOLATION_AWARE_ENABLED 1
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <commctrl.h>
#include <shellapi.h>
#include <stdio.h>
#include <tchar.h>
#include "md5.h"
#include "resource.h"

TCHAR gCmdLine[MAX_PATH]=_T("");
HINSTANCE gInstance=0;

const TCHAR good_msg[]=_T("MD5 Check Sums are the same.");
const TCHAR bad_msg[] =_T("MD5 Check Sums are different.");


void calc_md5(LPCTSTR filename, volatile BOOL *stop, TCHAR hex[33]) {
    int i,len;
    md5_byte_t buffer[512] = { 0 };
    md5_state_t state;
    md5_byte_t digest[16] = { 0 };
    FILE *pfile;

    if(lstrlen(filename) && (pfile=_tfopen(filename,_T("rb")))) {
        BOOL success = TRUE;
        md5_init(&state);

        do {
            if (stop && *stop) {
                success = FALSE;
                break;
            }
            len = fread(buffer, 1, sizeof(buffer), pfile);
            if (ferror(pfile)) {
                success = FALSE;
                break;
            }
            md5_append(&state, (const md5_byte_t *)buffer, len);
        } while(!feof(pfile));

        md5_finish(&state, digest);

        hex[0]=0;

        if (success) {
            for(i=0; i<16; i++) {
                _stprintf(hex, _T("%s%02x"), hex, digest[i]);
            }
        }

        fclose(pfile);
    }
}


BOOL CALLBACK MainDialogFunc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
    switch(uMsg) {

    case WM_INITDIALOG:
        {
            // make sure the right icon is on the window
            SendMessage(hwndDlg,WM_SETICON,ICON_SMALL,(LPARAM)LoadIcon(gInstance,MAKEINTRESOURCE(IDI_ICON1)));
            SetFocus(hwndDlg); // get up front

            SendMessage(GetDlgItem(hwndDlg,IDC_MD5COMPARE),EM_LIMITTEXT,32,0);

            if(lstrlen(gCmdLine)) { // if there is a command line
                SetDlgItemText(hwndDlg,IDC_FILENAME,gCmdLine);  // set it as the file
                PostMessage(hwndDlg, WM_COMMAND, ID_CALCULATE, 0); // and try to process it
            }
        }
        return TRUE;

    case WM_CLOSE:
        EndDialog(hwndDlg,1);
        break;

    case WM_DROPFILES:
        {
            TCHAR filename[MAX_PATH]=_T("");
            HDROP hDrop = (HDROP) wParam;
            DragQueryFile(hDrop,0,filename,MAX_PATH); // get the file name
            DragFinish(hDrop);                      // close the handle
            SetDlgItemText(hwndDlg,IDC_FILENAME,filename);
            PostMessage(hwndDlg,WM_COMMAND,ID_CALCULATE,0);
        }
        break;

    case WM_COMMAND:
        switch(LOWORD(wParam)) {
        case IDOK:
            EndDialog(hwndDlg,1);
            break;

        case IDC_FILENAME:
            if(HIWORD(wParam)==EN_CHANGE) {
                SetDlgItemText(hwndDlg,IDC_MD5SUM,_T(""));
            }
            break;

        case IDC_COMPARE:
            {
                TCHAR md5sum[33]=_T(""),md5sumcompare[33]=_T("");

                GetDlgItemText(hwndDlg,IDC_MD5SUM,md5sum,MAX_PATH);
                GetDlgItemText(hwndDlg,IDC_MD5COMPARE,md5sumcompare,MAX_PATH);

                if(lstrcmp(md5sum,md5sumcompare)) {
                    // failed
                    MessageBox(hwndDlg,bad_msg,_T(""),MB_OK|MB_ICONERROR);
                } else {
                    // success
                    MessageBox(hwndDlg,good_msg,_T(""),MB_OK|MB_ICONINFORMATION);
                }

            }
            break;

        case IDC_BROWSE:
            {
                TCHAR filename[MAX_PATH]=_T("");
                TCHAR mask[]=_T("*.*\0\0");
                OPENFILENAME open;

                ZeroMemory(&open,sizeof(OPENFILENAME));
                open.lStructSize  =(DWORD)sizeof(OPENFILENAME);
                open.lpstrFilter  =mask;
                open.nFilterIndex =0;
                open.lpstrFile    =filename;
                open.nMaxFile     =MAX_PATH;
                open.Flags        =OFN_EXPLORER;
                open.lpstrDefExt  =_T("");
                open.hwndOwner    =hwndDlg;
                open.hInstance    =gInstance;

                if(GetOpenFileName(&open)) {
                    SetDlgItemText(hwndDlg,IDC_FILENAME,filename);
                    PostMessage(hwndDlg,WM_COMMAND,ID_CALCULATE,0);
                }

            }
            break;

        case ID_CALCULATE:
            {
                TCHAR hex[33]=_T("");
                TCHAR filename[MAX_PATH]=_T("");
                GetDlgItemText(hwndDlg, IDC_FILENAME, filename, MAX_PATH);
                calc_md5(filename, NULL, hex);
                SetDlgItemText(hwndDlg,IDC_MD5SUM, hex[0] ? hex : _T("Invalid File"));
            }
            break;

        case IDC_ABOUT:
            ShellExecute(0,_T("open"),_T("https://github.com/ssrlive/winMD5"),0,0,SW_SHOWNORMAL);
            break;
        }
    }

    return FALSE;
};

void GetParam()
{
    int si=0,di=0;

    TCHAR *cmdLine=GetCommandLine();
    TCHAR exeName[MAX_PATH];

    GetModuleFileName(gInstance,exeName,MAX_PATH);

    // skip past the module name
    if(_tcsstr(cmdLine,exeName)) {
        if(cmdLine[0]=='"'&&exeName[0]!='"') si=1;
        si+=lstrlen(exeName);
        while(isspace(cmdLine[si])||cmdLine[si]=='"') si++;
    }

    if(si<lstrlen(cmdLine)) {
        while(cmdLine[si]) {
            if(cmdLine[si]!='"') gCmdLine[di++]=cmdLine[si];
            si++;
        }
    }
    gCmdLine[di]=0;
};

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLineA,int nCmdShow) {
    gInstance=hInstance;
    GetParam();
    InitCommonControls();
    return DialogBox(hInstance,MAKEINTRESOURCE(IDD_MAIN),0,(DLGPROC)MainDialogFunc);
};
