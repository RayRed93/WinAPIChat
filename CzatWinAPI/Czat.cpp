#include <winsock2.h>
#include <windows.h>
#include <tchar.h>
#include <CommCtrl.h>

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"Winmm.lib")


#define IDC_EDIT_IN				101
#define IDC_EDIT_OUT			102
#define IDC_BUTTON_SEND			103
#define IDC_BUTTON_RUN_SERVER   104
#define IDC_EDIT_PORT			105
#define IDC_STATIC				106	
#define IDC_EDIT_NICKNAME		107
#define IDC_BUTTON_CONNECT		108 
#define IDC_EDIT_SERVER_ADDRESS 109
#define IDC_STATIC2				110
#define IDC_STATIC3				111
#define IDC_BUTTON_CHECKBOX		112

HWND hEditIn = NULL;
HWND hEditOut = NULL;
HWND hEditPort = NULL;
HWND hEditServer = NULL;
HWND hEditNick = NULL;
HWND hSendButton = NULL;
HWND hServerButton = NULL;
HWND hConnectButton = NULL;
HWND hEnterCheckbox = NULL;
SOCKET listenSocket = INVALID_SOCKET;
SOCKET connectionSocket = INVALID_SOCKET;
HANDLE hServerThread = NULL;
HANDLE hClientThread = NULL;
HANDLE hKeyThread = NULL;
BOOL serwerDziala = FALSE;
BOOL klientDziala = FALSE;

#define ROZMIAR_BUFORA  1024
#define SMALL_BUFF 64

LRESULT CALLBACK WinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


void DodajKomunikat(HWND hEdit, TCHAR *tekst, TCHAR *source)
{
	TCHAR buf[ROZMIAR_BUFORA];
	int index = GetWindowTextLength(hEdit);
	_tcscpy_s(buf, ROZMIAR_BUFORA, tekst);
	//_tcscat_s(buf, ROZMIAR_BUFORA, TEXT("\n"));
	//source = (TCHAR*)malloc(sizeof(TCHAR) * 20);
	if (source == NULL) source = TEXT("INFO");
	_stprintf_s(buf, ROZMIAR_BUFORA, TEXT("[%s]: %s\n"), source, tekst);
	
	SetFocus(hEditOut);

	SendMessage(hEdit, EM_SETSEL, (WPARAM)index, (LPARAM)index);
	SendMessage(hEdit, EM_REPLACESEL, 0, (LPARAM)buf);
	
	
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd)
{
	WNDCLASSEX wClass;
	ZeroMemory(&wClass, sizeof(WNDCLASSEX));
	wClass.cbSize = sizeof(WNDCLASSEX);
	wClass.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wClass.hInstance = hInst;
	wClass.lpfnWndProc = (WNDPROC)WinProc;
	wClass.lpszClassName = TEXT("Window Class");
	wClass.style = CS_HREDRAW | CS_VREDRAW;

	if (!RegisterClassEx(&wClass))
	{
		MessageBox(NULL, TEXT("Window class creation failed"), TEXT("Window Class Failed"), MB_ICONERROR);
		return 1;
	}

	HWND hWnd = CreateWindow(
		TEXT("Window Class"),
		TEXT("WinAPIChat"),
		WS_OVERLAPPEDWINDOW,
		200,
		200,
		335,
		480,
		NULL,
		NULL,
		hInst,
		NULL);

	if (!hWnd)
		MessageBox(NULL, TEXT("Window creation failed"), TEXT("Window Creation Failed"), MB_ICONERROR);

	ShowWindow(hWnd, nShowCmd);

	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}



DWORD ServerThread(LPVOID dane)
{
	SOCKET *socket = (SOCKET*)dane;
	TCHAR tekst[ROZMIAR_BUFORA];
	TCHAR myNick[64];
	TCHAR clientNick[64];
	int iResult = 0;

	DodajKomunikat(hEditIn, TEXT("Czekam na nowe połączenie..."), NULL);

	connectionSocket = accept(*socket, NULL, NULL);
	if (connectionSocket == INVALID_SOCKET) {
		DodajKomunikat(hEditIn, TEXT("Accept fail.\n"), NULL);
		closesocket(*socket);
		serwerDziala = FALSE;
		return 1;
	}

	DodajKomunikat(hEditIn, TEXT("Klient połączony!"), NULL);

	EnableWindow(hEditOut, TRUE);
	EnableWindow(hSendButton, TRUE);
	/*SOCKADDR_IN peerAdress;
	int l = sizeof(peerAdress);
	getpeername(connectionSocket, (SOCKADDR*)&peerAdress, &l);
	DodajKomunikat(hEditIn, (TCHAR*)inet_ntoa(peerAdress.sin_addr));*/
	
	GetWindowText(hEditNick, LPWSTR(myNick), sizeof(myNick));
	iResult = send(connectionSocket, (char*)myNick, sizeof(myNick), 0);    //Wymiana nazw (serwer wysyla pierwszy)
	iResult = recv(connectionSocket, (char*)clientNick, sizeof(tekst), 0);
	

	

	while (serwerDziala)
	{
		ZeroMemory(tekst, sizeof(tekst));
	
		iResult = recv(connectionSocket, (char *)tekst, sizeof(tekst), 0);
		if (iResult > 0)
		{
			DodajKomunikat(hEditIn, tekst, clientNick);
			PlaySound(TEXT("DeviceConnect"), NULL, SND_ALIAS | SND_ASYNC);
		}
		else
		{
			DodajKomunikat(hEditIn, TEXT("Connection closed"), NULL);
			EnableWindow(hEditOut, FALSE);
			EnableWindow(hSendButton, FALSE);
			break;
		}
	}
	EnableWindow(hConnectButton, TRUE);
	EnableWindow(hEditOut, FALSE);
	EnableWindow(hSendButton, FALSE);
	closesocket(connectionSocket);
	return 0;
}

DWORD ClientThread(LPVOID dane)
{
	//SOCKET *connectSocket = (SOCKET*)dane;
	TCHAR recvbuf[ROZMIAR_BUFORA];
	int recvbuflen = ROZMIAR_BUFORA;
	TCHAR connectServerAddr[64];
	TCHAR connectServerPort[10];
	TCHAR serverNick[64];
	TCHAR myNick[64];

	WSADATA wsaData;
	int iResult;

	connectionSocket = INVALID_SOCKET;
	struct sockaddr_in clientService;
	TCHAR sendbuff[ROZMIAR_BUFORA];

	

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (iResult != NO_ERROR)
	{
		TCHAR* komunikat;
		_stprintf_s(komunikat, 512, TEXT("WSAStartup failed: %d\n"), iResult);
		DodajKomunikat(hEditIn, komunikat, NULL);
		klientDziala = FALSE;
		return 1;
	}

	connectionSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (connectionSocket == INVALID_SOCKET)
	{
		TCHAR* komunikat;
		_stprintf_s(komunikat, SMALL_BUFF, TEXT("Error at socket(): %d\n"), WSAGetLastError());
		WSACleanup();
		klientDziala = FALSE;
		return 1;
	}

	GetWindowText(hEditPort, LPWSTR(connectServerPort), sizeof(connectServerPort));
	DWORD ip_addr;
	SendMessage(hEditServer, IPM_GETADDRESS, 0, (LPARAM)&ip_addr);

	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = htonl(ip_addr); //SERVER ADRESS
	clientService.sin_port = htons(_ttoi(connectServerPort));  // PORT SERVER

	iResult = connect(connectionSocket, (SOCKADDR*)&clientService, sizeof(clientService));

	if (iResult == SOCKET_ERROR)
	{
		
		TCHAR komunikat[ROZMIAR_BUFORA];
		_stprintf_s(komunikat, 512, TEXT("Unable to connect to server: %d\n"), WSAGetLastError());
		DodajKomunikat(hEditIn, komunikat, NULL);
		closesocket(connectionSocket);
		WSACleanup();
		klientDziala = FALSE;
		return 1;
	}
	
	DodajKomunikat(hEditIn, TEXT("Połączono z serwerem!"), NULL);

	SetWindowText(hConnectButton, TEXT("Rozłącz z serwerem"));
	EnableWindow(hEditOut, TRUE);
	EnableWindow(hSendButton, TRUE);
	EnableWindow(hServerButton, FALSE);
	//EnableWindow(hConnectButton, FALSE);

	/*iResult = send(ConnectSocket, (char*)sendbuff, sizeof(sendbuff), 0);

	if (iResult == SOCKET_ERROR)
	{
		closesocket(ConnectSocket);
		TCHAR* komunikat;
		_stprintf_s(komunikat, 512, TEXT("Send faild: %d\n"), WSAGetLastError());
		WSACleanup();
		return 1;
	}*/
	GetWindowText(hEditNick, LPWSTR(myNick), sizeof(myNick));
	iResult = recv(connectionSocket, (char*)serverNick, sizeof(serverNick), 0);
	iResult = send(connectionSocket, (char*)myNick, sizeof(myNick), 0);    //Wymiana nazw (serwer wysyla pierwszy)

	while(klientDziala)
	{
		ZeroMemory(recvbuf, sizeof(recvbuf));
		
		iResult = recv(connectionSocket, (char*)recvbuf, sizeof(recvbuf), 0);
	

		if (iResult > 0)
		{
			DodajKomunikat(hEditIn, recvbuf, serverNick);
			PlaySound(TEXT("DeviceConnect"), NULL, SND_ALIAS | SND_ASYNC);
		}		
		else
		{
			DodajKomunikat(hEditIn, TEXT("Połączenie zakończone."), NULL);
			break;
		}

	}

	closesocket(connectionSocket);
	WSACleanup();
	EnableWindow(hConnectButton, TRUE);
	EnableWindow(hEditOut, FALSE);
	EnableWindow(hSendButton, FALSE);
	EnableWindow(hServerButton, TRUE);
	
	return 0;

}

void SendMessageSOCKET()
{
	int n = 0;
	TCHAR myNick[64];
	TCHAR tekst[ROZMIAR_BUFORA];

	ZeroMemory(tekst, sizeof(tekst));
	GetWindowText(hEditOut, tekst, ROZMIAR_BUFORA);
	GetWindowText(hEditNick, myNick, sizeof(myNick));
	n = send(connectionSocket, (char*)tekst, sizeof(tekst), 0);
	if (n == SOCKET_ERROR)
		DodajKomunikat(hEditIn, TEXT("Błąd wysyłania!"), NULL);

	DodajKomunikat(hEditIn, tekst, myNick);
	SetWindowText(hEditOut, TEXT(""));
}

DWORD KeyThread(LPVOID dane)
{
	int keystate;
	while (true)
	{
		keystate = GetKeyState(VK_RETURN);
		if (keystate < 0 && (klientDziala || serwerDziala) && connectionSocket != INVALID_SOCKET)
		{
			SendMessageSOCKET();
			Sleep(200);
		}
		Sleep(20);
	}
	return 0;

}



LRESULT CALLBACK WinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_BUTTON_CONNECT:
		{
				
			if (klientDziala == FALSE)
			{
				hClientThread = CreateThread(NULL, 0,
					(LPTHREAD_START_ROUTINE)ClientThread, NULL, 0, NULL);

				if (hClientThread == NULL)
				{
					MessageBox(hWnd, TEXT("Create thread failed"), TEXT("Error"), MB_OK);
					break;
				}
				klientDziala = TRUE;
				
			}
			else
			{
				klientDziala = FALSE;

				EnableWindow(hConnectButton, TRUE);
				EnableWindow(hServerButton, TRUE);
				EnableWindow(hEditOut, FALSE);
				EnableWindow(hSendButton, FALSE);
				//closesocket(connectionSocket);
				//connectionSocket = INVALID_SOCKET;	
				DodajKomunikat(hEditIn, TEXT("Połaczenie przerwane."), NULL);
				SetWindowText(GetDlgItem(hWnd, IDC_BUTTON_CONNECT), TEXT("Po³¹cz z serwerem"));

			}
			
			break;
		}
		case IDC_BUTTON_RUN_SERVER:
		{
			TCHAR komunikat[ROZMIAR_BUFORA];

			if (serwerDziala == FALSE)
			{
				int nPort = 0;
				TCHAR portstr[100];
				SOCKADDR_IN adres;

				if (!GetDlgItemText(hWnd, IDC_EDIT_PORT, portstr, 100)) portstr[0] = TEXT('\0');
				else nPort = _ttoi(portstr);

				_stprintf_s(komunikat, ROZMIAR_BUFORA, TEXT("Próba połącznia na porcie %d"), nPort);
				DodajKomunikat(hEditIn, komunikat, NULL);

				listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
				if (listenSocket == INVALID_SOCKET) {
					MessageBox(hWnd, TEXT("Socket error"), TEXT("Error"), MB_OK);
					break;
				}

				adres.sin_port = htons(nPort);
				adres.sin_family = AF_INET;
				adres.sin_addr.s_addr = htonl(INADDR_ANY);

				if (bind(listenSocket, (SOCKADDR*)&adres, sizeof(adres)) == SOCKET_ERROR)
				{
					MessageBox(hWnd, TEXT("Bind error"), TEXT("Error"), MB_OK);
					closesocket(listenSocket);
					break;
				}

				if (listen(listenSocket, 1) == SOCKET_ERROR)
				{
					MessageBox(hWnd, TEXT("Listen error"), TEXT("Error"), MB_OK);
					closesocket(listenSocket);
					break;
				}

				hServerThread = CreateThread(NULL, 0,
					(LPTHREAD_START_ROUTINE)ServerThread, &listenSocket, 0, NULL);

				if (hServerThread == NULL)
				{
					MessageBox(hWnd, TEXT("Create thread failed"), TEXT("Error"), MB_OK);
					closesocket(listenSocket);
					break;
				}

				
				SetWindowText(GetDlgItem(hWnd, IDC_BUTTON_RUN_SERVER), TEXT("Zatrzymaj serwer"));
				serwerDziala = TRUE;
				EnableWindow(hConnectButton, FALSE);
			}
			else
			{
				EnableWindow(hConnectButton, TRUE);
				EnableWindow(hEditOut, FALSE);
				EnableWindow(hSendButton, FALSE);
				closesocket(listenSocket);
				listenSocket = INVALID_SOCKET;
				serwerDziala = FALSE;
				DodajKomunikat(hEditIn, TEXT("Serwer zatrzymany."), NULL);
				SetWindowText(GetDlgItem(hWnd, IDC_BUTTON_RUN_SERVER), TEXT("Uruchom serwer"));
				
			}
			break;
		}
		case IDC_BUTTON_SEND:
		{
			SendMessageSOCKET();
			break;
		}
		case IDC_BUTTON_CHECKBOX:
		{
			BOOL checked = IsDlgButtonChecked(hWnd, IDC_BUTTON_CHECKBOX);
			if (checked) {
				CheckDlgButton(hWnd, IDC_BUTTON_CHECKBOX, BST_UNCHECKED);
				SuspendThread(hKeyThread);
			}
			else {
				CheckDlgButton(hWnd, IDC_BUTTON_CHECKBOX, BST_CHECKED);
				ResumeThread(hKeyThread);
			}
			break;
		}
		}
		break;
	case WM_CREATE:
	{
		WSADATA WsaDat;
		HGDIOBJ hfDefault;

		CreateWindow(TEXT("STATIC"), TEXT("Port: "),
			WS_CHILD | WS_VISIBLE,
			10, 40, 30, 23, hWnd, (HMENU)IDC_STATIC, GetModuleHandle(NULL), NULL);
		CreateWindow(TEXT("STATIC"), TEXT("Nick: "),
			WS_CHILD | WS_VISIBLE,
			10, 10, 30, 23, hWnd, (HMENU)IDC_STATIC2, GetModuleHandle(NULL), NULL);

		CreateWindow(TEXT("STATIC"), TEXT("Serv: "),
			WS_CHILD | WS_VISIBLE,
			10, 70, 30, 23, hWnd, (HMENU)IDC_STATIC3, GetModuleHandle(NULL), NULL);
		hEditPort = CreateWindow(TEXT("EDIT"), TEXT("5841"),
			WS_CHILD | WS_VISIBLE | WS_BORDER,
			40, 40, 60, 23, hWnd, (HMENU)IDC_EDIT_PORT, GetModuleHandle(NULL), NULL);

		hServerButton = CreateWindow(TEXT("BUTTON"), TEXT("Uruchom serwer"),
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
			170, 40, 130, 23,
			hWnd,
			(HMENU)IDC_BUTTON_RUN_SERVER,
			GetModuleHandle(NULL),
			NULL);

		hEditNick = CreateWindow(TEXT("EDIT"), TEXT("user"),
			WS_CHILD | WS_VISIBLE | WS_BORDER,
			40, 10, 60, 23, hWnd, (HMENU)IDC_EDIT_NICKNAME, GetModuleHandle(NULL), NULL);
		
		hEditServer = CreateWindow(WC_IPADDRESS, TEXT("127.0.0.1"),
			WS_CHILD | WS_VISIBLE | WS_BORDER,
			40, 70, 120, 23, hWnd, (HMENU)IDC_EDIT_SERVER_ADDRESS, GetModuleHandle(NULL), NULL);

		hConnectButton = CreateWindow(TEXT("BUTTON"), TEXT("Połącz z serwerem"),
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
			170, 70, 130, 23,
			hWnd,
			(HMENU)IDC_BUTTON_CONNECT,
			GetModuleHandle(NULL),
			NULL);


		hEnterCheckbox = CreateWindowEx(NULL,TEXT("BUTTON"), TEXT("Wysyłaj enterem"),
			WS_VISIBLE | WS_CHILD | BS_CHECKBOX,
			170, 10, 130, 23,
			hWnd,
			(HMENU)IDC_BUTTON_CHECKBOX,
			GetModuleHandle(NULL),
			NULL);

		hEditIn = CreateWindowEx(WS_EX_CLIENTEDGE,
			TEXT("EDIT"), TEXT(""),
			WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | WS_VSCROLL,
			10, 120, 300,200,
			hWnd,
			(HMENU)IDC_EDIT_IN,
			GetModuleHandle(NULL),
			NULL);

		if (!hEditIn)  MessageBox(hWnd, TEXT("Could not create incoming edit box."), TEXT("Error"), MB_OK | MB_ICONERROR);

		hEditOut = CreateWindowEx(WS_EX_CLIENTEDGE,
			TEXT("EDIT"), TEXT("Tutaj wpisz wiadomoœæ..."), WS_CHILD | WS_VISIBLE | ES_MULTILINE |
			ES_AUTOVSCROLL | ES_AUTOHSCROLL,
			10, 360, 220, 60, hWnd,
			(HMENU)IDC_EDIT_OUT,
			GetModuleHandle(NULL),
			NULL);

		if (!hEditOut) MessageBox(hWnd, TEXT("Could not create outgoing edit box."), TEXT("Error"), MB_OK | MB_ICONERROR);


		hSendButton = CreateWindow(
			TEXT("BUTTON"),
			TEXT("Wyœlij"),
			WS_TABSTOP | WS_VISIBLE |
			WS_CHILD | BS_DEFPUSHBUTTON,
			240, 360, 60, 60,
			hWnd,
			(HMENU)IDC_BUTTON_SEND,
			GetModuleHandle(NULL),
			NULL);

		EnableWindow(hEditOut, FALSE);
		EnableWindow(GetDlgItem(hWnd, IDC_BUTTON_SEND), FALSE);

		if (WSAStartup(MAKEWORD(2, 2), &WsaDat) != 0)
		{
			MessageBox(hWnd, TEXT("Winsock initialization failed"), TEXT("Critical Error"), MB_ICONERROR);
			SendMessage(hWnd, WM_DESTROY, 0, 0);
			break;
		}

		hfDefault = GetStockObject(DEFAULT_GUI_FONT);
		SendMessage(GetDlgItem(hWnd, IDC_EDIT_PORT), WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
		SendMessage(GetDlgItem(hWnd, IDC_STATIC2), WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
		SendMessage(GetDlgItem(hWnd, IDC_EDIT_SERVER_ADDRESS), WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
		SendMessage(GetDlgItem(hWnd, IDC_STATIC), WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
		SendMessage(hEditOut, WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
		SendMessage(hEditIn, WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
		SendMessage(hSendButton, WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
		SendMessage(GetDlgItem(hWnd, IDC_BUTTON_RUN_SERVER), WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
		SendMessage(GetDlgItem(hWnd, IDC_BUTTON_CONNECT), WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
		SendMessage(GetDlgItem(hWnd, IDC_EDIT_NICKNAME), WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
		SendMessage(GetDlgItem(hWnd, IDC_STATIC3), WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
		SendMessage(GetDlgItem(hWnd, IDC_BUTTON_CHECKBOX), WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
		SendMessage(hEditServer, IPM_SETADDRESS, 0, MAKEIPADDRESS(127, 0, 0, 1));

		hKeyThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)KeyThread, NULL, CREATE_SUSPENDED, NULL);
		
	}
	break;

	case WM_DESTROY:
	{
		PostQuitMessage(0);
		WSACleanup();
		return 0;
	}
	break;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}