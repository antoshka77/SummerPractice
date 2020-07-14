#if defined(UNICODE) && !defined(_UNICODE)
    #define _UNICODE
#elif defined(_UNICODE) && !defined(UNICODE)
    #define UNICODE
#endif



#include <windows.h>
#include <stdio.h>
#include <locale.h>
#include <string.h>

/*  Declare Windows procedure  */
#include "menu.h"


#define M_DEF 0
#define M_LAY 1


typedef struct
{
	char* strings;//текст
	int size;//размер буффера
}savedata;//модель хранения

typedef struct
{
	int* strings;//индексы концов строк
	int* stringsWithLayout;
	int maxLen;//максимальная длина строк
	int countOfStr;//количество строк
	int mode;//режим
	int countOfStrWithLayout;//количество строк в режиме верстки

}modeldata;//модель представления

typedef struct
{
	int cxChar;//ширина символа
	int cyChar;//длина символа
	int cxClient;//ширина окна
	int cyClient;//высота окна
	int iMaxWidth;//максимальная высота
	int iVscrollPos;//позиция бегунка по вертикали
	int iVscrollMax;//максимальная позицияи бегунка по вертикали
	int iHscrollPos;//позиция бегунка по горизнотали
	int iHscrollMax;//максимальная позиция бегунка по горизонтали
	int lenLayout;//длина строки в режиме верстки
}metrixdata;//метрики

//создание меню
HMENU CreateMyMenu(HWND hwnd)
{
	HMENU hMenu;
	HMENU hMenuPopup;
	hMenu = CreateMenu();
	hMenuPopup = CreateMenu();
	AppendMenu(hMenuPopup, MF_STRING, IDM_OPEN, "&Open...");
	AppendMenu(hMenuPopup, MF_SEPARATOR, 0, NULL);
	AppendMenu(hMenuPopup, MF_STRING, IDM_EXIT, "Exit");
	AppendMenu(hMenu, MF_POPUP, (UINT)hMenuPopup, "&File");
	hMenuPopup = CreateMenu();
	AppendMenu(hMenuPopup, MF_STRING | MF_CHECKED, IDM_DEFAULT, "&Default");
	AppendMenu(hMenuPopup, MF_SEPARATOR, 0, NULL);
	AppendMenu(hMenuPopup, MF_STRING | MF_UNCHECKED, IDM_LAYOUT, "&Layout");
	AppendMenu(hMenu, MF_POPUP, (UINT)hMenuPopup, "&Mode");
	SetMenu(hwnd, hMenu);
	printf("MENU IS CREATE ");
	return hMenu;
}

//чтение текста, заполнение модели хранения
//вход: имя файла
//выход: модель хранения
savedata ReadText(char* fileName)
{
	savedata sData;
	FILE* f;
	int size = 0;
	int readed = 0;
	f = fopen(fileName, "rb");
	if (f != NULL)
	{
		fseek(f, 0, SEEK_END);
		size = ftell(f);
		fseek(f, 0, SEEK_SET);
		sData.strings = (char*)calloc(size + 1, 1);
		if (sData.strings != NULL)
		{
			readed = fread(sData.strings, 1, size + 1, f);
			sData.strings[size] = '\0';
			if (readed != size)
			{
				printf("File not readed fully\n");
			}
			else
				sData.size = size;
		}
		else
			printf("buf is not calloc");
		fclose(f);
	}
	else
		printf("file is not open");
	return sData;
}

//инициализация модели представления
//вход: модель хранения, режим работы
//выход: модель представления
modeldata InitModelData(savedata sData, int mode)
{
	modeldata mData;
	int countOfInd = 2;
	int i;
	int* curPtr;
	i = 1;
	mData.mode = mode;
	mData.strings = (int*)malloc(sizeof(int));
	mData.stringsWithLayout = (int*)NULL;
	//mData.stringsWithLayout = (int*)malloc((sData.size + 1) * sizeof(int));

	mData.strings[0] = -1;
	if (mData.strings != NULL)
	{
		while (i < sData.size)
		{
			if (sData.strings[i] == '\n')
			{
				curPtr = (int*)realloc(mData.strings, countOfInd * sizeof(int));
				if (curPtr != NULL)
				{
					mData.strings = curPtr;
				}
				/*else
				{
					free(mData.strings);
					return mData;//проинитить ее null и прочим
				}*/
				mData.strings[countOfInd - 1] = i;
				if (countOfInd == 2)
				{
					mData.maxLen = i;
				}
				countOfInd++;
			}
			if (countOfInd >= 4)
			{
				if (mData.strings[countOfInd - 2] - mData.strings[countOfInd - 3] - 1 > mData.maxLen)
				{
					mData.maxLen = mData.strings[countOfInd - 2] - mData.strings[countOfInd - 3] - 1;
				}
			}
			i++;
		}
		curPtr = (int*)realloc(mData.strings, countOfInd * sizeof(int));
		if (curPtr != NULL)
		{
			//free(mData.strings);
			mData.strings = curPtr;
		}
		mData.countOfStr = countOfInd;
		mData.strings[countOfInd - 1] = sData.size;
		if (mData.strings[countOfInd - 1] - mData.strings[countOfInd - 2] - 1 > mData.maxLen)
			mData.maxLen = mData.strings[countOfInd - 2] - mData.strings[countOfInd - 3] - 1;
		mData.countOfStrWithLayout = 0;

	}
		return mData;
}

//печать текста
//вход: начальная и конечная позиции, буффер на чтение, массив смещений строк, метрики, хэндлер
//выход: отображение текста
void PrintText(int iPaintBeg, int iPaintEnd, char* buf, int* arrOfStrings, metrixdata metrix, HDC hdc, int mode)
{
	int x, y, i;
	for (i = iPaintBeg; i < iPaintEnd; i++)
	{
		x = metrix.cxChar * (1 - metrix.iHscrollPos);
		y = metrix.cyChar * (1 - metrix.iVscrollPos + i);//здесь так же зависит от i, поэтому выводится строка
		if (mode == M_DEF)
		{
			TextOut(
				hdc, x, y,
				(buf + arrOfStrings[i] + 1),
				((arrOfStrings[i + 1] - arrOfStrings[i]) - 1)
			);
		}
		if (mode == M_LAY)
		{
			TextOut(
				hdc, x, y,
				(buf + arrOfStrings[i]),
				((arrOfStrings[i + 1] - arrOfStrings[i]))
			);
		}
	}
}

//бинарный поиск
//вход: размер буффера, буффер, ключ
//выход: индекс ближайшего права элемента
int BinarySearch(int size, int arr[], int key)
{
	int first = 0;
	int last = size;
	int mid;
	while (first < last) {
		mid = first + (last - first) / 2;
		if (key <= arr[mid])
			last = mid;
		else
			first = mid + 1;
	}
	return last;
}

int maximum(int a, int b)
{
	if (a > b)
		return a;
	else
		return b;
}

int minimum(int a, int b)
{
	if (a < b)
		return a;
	else
		return b;
}

//подсчет количества строк в режиме верстки
//вход: размер буффера, буффер исходных смещений, длина строки
//выход: количество строк в режиме верстки
int CountOfStr(int size, int arr[], int lenOfStr)
{
	int i;
	int ind = 0;
	int curLenOfStr;
	for (i = 0; i < size - 1; i++)
	{
		curLenOfStr = arr[i + 1] - arr[i] - 1;
		ind += (curLenOfStr / lenOfStr + 1);
	}
	return ind;
}

//заполнение буффера смещений в режиме верстки
//вход: размер исходного буффера, исходный буффер, указатель на новый буфффер, длина строки
//выход: количество строк в новой буффере, новый буффер
int SetLayoutArr(int size, int arr[], int* arrLayout, int lenOfStr)
{
	int curLenOfStr;
	int countOfStr = 1;
	int i;
	int j;
	arrLayout[0] = 0;
	for (i = 1; i < size; i++)
	{
		j = 1;
		curLenOfStr = arr[i] - arr[i - 1] - 1;
		if (curLenOfStr <= lenOfStr)
		{
			arrLayout[countOfStr] = arr[i];
			countOfStr++;
		}
		else
		{
			while (curLenOfStr > lenOfStr)
			{
				arrLayout[countOfStr] = arr[i - 1] + lenOfStr * j + 1;
				j++;
				curLenOfStr -= lenOfStr;
				countOfStr++;
			}
			arrLayout[countOfStr] = arr[i];
			if (arrLayout[countOfStr] - arrLayout[countOfStr - 1] <= 1)
			{
				arrLayout[countOfStr - 1] = arr[i];
				countOfStr--;
			}
			countOfStr++;
		}
	}
	//arrLayout[countOfStr] = size;
	return countOfStr;
}

modeldata SetLayoutArr1(modeldata mData, int lenOfStr)//mData.countOfStrWithLayout = SetLayoutArr(mData.countOfStr, mData.strings, mData.stringsWithLayout, metrix.lenLayout);
{
	modeldata mDataRes = mData;
	int* curPtr;
	int curLenOfStr;
	int countOfStr = 1;
	int i;
	int j;
	if (mData.stringsWithLayout != NULL)
		free(mData.stringsWithLayout);
	mDataRes.stringsWithLayout = (int*)malloc(sizeof(int));
	if (mDataRes.stringsWithLayout != NULL)
	{
		mDataRes.stringsWithLayout[0] = 0;
		for (i = 1; i < mData.countOfStr; i++)
		{
			j = 1;
			curLenOfStr = mDataRes.strings[i] - mDataRes.strings[i - 1] - 1;
			curPtr = (int*)realloc(mDataRes.stringsWithLayout, (countOfStr + 1) * sizeof(int));
			if (curPtr != NULL)
			{
				mDataRes.stringsWithLayout = curPtr;
				if (curLenOfStr <= lenOfStr)
				{
					mDataRes.stringsWithLayout[countOfStr] = mDataRes.strings[i];
					countOfStr++;
				}
				else
				{
					while (curLenOfStr > lenOfStr)
					{
						curPtr = (int*)realloc(mDataRes.stringsWithLayout, (countOfStr + 1) * sizeof(int));
						if (curPtr != NULL)
						{
							mDataRes.stringsWithLayout = curPtr;
							mDataRes.stringsWithLayout[countOfStr] = mDataRes.strings[i - 1] + lenOfStr * j + 1;
							j++;
							curLenOfStr -= lenOfStr;
							countOfStr++;
						}
						else
						{
							mDataRes.countOfStrWithLayout = 0;
							free(mDataRes.stringsWithLayout);
							return mDataRes;
						}
					}
					curPtr = (int*)realloc(mDataRes.stringsWithLayout, (countOfStr + 1) * sizeof(int));
					if (curPtr != NULL)
					{
						mDataRes.stringsWithLayout = curPtr;
						mDataRes.stringsWithLayout[countOfStr] = mDataRes.strings[i];
					}
					else
					{
						mDataRes.countOfStrWithLayout = 0;
						free(mDataRes.stringsWithLayout);
						return mDataRes;
					}
					if (mDataRes.stringsWithLayout[countOfStr] - mDataRes.stringsWithLayout[countOfStr - 1] <= 1)
					{
						mDataRes.stringsWithLayout[countOfStr - 1] = mDataRes.strings[i];
						countOfStr--;
					}
					countOfStr++;
				}
			}
			else
			{
				mDataRes.countOfStrWithLayout = 0;
				free(mDataRes.stringsWithLayout);
				return mDataRes;
			}
		}
		/*curPtr = (int*)realloc(mDataRes.stringsWithLayout, (countOfStr + 1) * sizeof(int));
		if (curPtr != NULL)
		{
			mDataRes.stringsWithLayout = curPtr;
			mDataRes.stringsWithLayout[countOfStr] = mDataRes.countOfStr;
		}
		else
		{
			mDataRes.countOfStrWithLayout = 0;
			free(mDataRes.stringsWithLayout);
			return mDataRes;
		}*/
	}
	else
		mDataRes.countOfStrWithLayout = 0;
	mDataRes.countOfStrWithLayout = countOfStr;
	return mDataRes;
}

int CountOfStrWithLayout(int size, int arr[], int lenOfStr)
{
	int curLenOfStr;
	int countOfStr = 1;
	int i;
	int j;
	for (i = 1; i < size; i++)
	{
		j = 1;
		curLenOfStr = arr[i] - arr[i - 1] - 1;
		if (curLenOfStr <= lenOfStr)
		{
			//arrLayout[countOfStr] = arr[i];
			countOfStr++;
		}
		else
		{
			while (curLenOfStr > lenOfStr)
			{
				//arrLayout[countOfStr] = arr[i - 1] + lenOfStr * j + 1;
				j++;
				curLenOfStr -= lenOfStr;
				countOfStr++;
			}
			//arrLayout[countOfStr] = arr[i];
			/*if (arrLayout[countOfStr] - arrLayout[countOfStr - 1] <= 1)
			{
				 arrLayout[countOfStr - 1] = arr[i];
				 countOfStr--;
			}*/
			countOfStr++;
		}
	}
	//arrLayout[countOfStr] = size;
	return countOfStr;
}

modeldata SetLayoutArr2(modeldata mData, int lenOfStr)//mData.countOfStrWithLayout = SetLayoutArr(mData.countOfStr, mData.strings, mData.stringsWithLayout, metrix.lenLayout);
{
	modeldata mDataRes = mData;
	//int* curPtr = mData.S;
	int curLenOfStr;
	int countOfStr = 1;
	int i;
	int j;
	if (mData.stringsWithLayout != NULL)
		free(mData.stringsWithLayout);
	countOfStr = CountOfStrWithLayout(mDataRes.countOfStr, mDataRes.strings, lenOfStr);
	mDataRes.stringsWithLayout = (int*)malloc(countOfStr * sizeof(int));
	if (lenOfStr < 20)
	{
		free(mDataRes.stringsWithLayout);
		mDataRes.stringsWithLayout = NULL;
	}
	countOfStr = 1;
	if (mDataRes.stringsWithLayout != NULL)
	{
		mDataRes.stringsWithLayout[0] = 0;
		for (i = 1; i < mData.countOfStr; i++)
		{
			j = 1;
			curLenOfStr = mDataRes.strings[i] - mDataRes.strings[i - 1] - 1;
			//curPtr = (int*)realloc(mDataRes.stringsWithLayout, (countOfStr + 1) * sizeof(int));
			if (curLenOfStr <= lenOfStr)
			{
				mDataRes.stringsWithLayout[countOfStr] = mDataRes.strings[i];
				countOfStr++;
			}
			else
			{
				while (curLenOfStr > lenOfStr)
				{
					mDataRes.stringsWithLayout[countOfStr] = mDataRes.strings[i - 1] + lenOfStr * j + 1;
					j++;
					curLenOfStr -= lenOfStr;
					countOfStr++;
				}
				mDataRes.stringsWithLayout[countOfStr] = mDataRes.strings[i];
				if (mDataRes.stringsWithLayout[countOfStr] - mDataRes.stringsWithLayout[countOfStr - 1] <= 1)
				{
					mDataRes.stringsWithLayout[countOfStr - 1] = mDataRes.strings[i];
					countOfStr--;
				}
				countOfStr++;
			}
		}
			/*curPtr = (int*)realloc(mDataRes.stringsWithLayout, (countOfStr + 1) * sizeof(int));
			if (curPtr != NULL)
			{
				mDataRes.stringsWithLayout = curPtr;
				mDataRes.stringsWithLayout[countOfStr] = mDataRes.countOfStr;
			}
			else
			{
				mDataRes.countOfStrWithLayout = 0;
				free(mDataRes.stringsWithLayout);
				return mDataRes;
			}*/
		mDataRes.countOfStrWithLayout = countOfStr;
	}
	else
		mDataRes.countOfStrWithLayout = 0;
	return mDataRes;
}

//установить горизонтельный скролл
//вход: смещение, максимальное значение, текущее значение, ширина символа, хэндлер
//выход: текущее положение скролла
int SetHScroll(int iHscrollInc, int hMax, int hPos, int cxChar, HWND hwnd)
{
	iHscrollInc = maximum(
		-hPos,
		minimum(iHscrollInc, hMax - hPos)
	);
	if (iHscrollInc != 0)
	{
		hPos += iHscrollInc;
		ScrollWindow(hwnd, -cxChar * iHscrollInc, 0, NULL, NULL);
		SetScrollPos(hwnd, SB_HORZ, hPos, TRUE);
	}
	return hPos;
}

//инициализация открывателя файлов
//вход: структура открывателя файлов, хэндлер
//выход: указатель на открыватель файлов
void InitOFN(OPENFILENAME* ofn, HWND* hwnd)
{
	ZeroMemory(ofn, sizeof(*ofn));
	static CHAR szFilter[] = "Text Files(*.txt)\0*.txt\0All Files(*.*)\0*.*\0\0";
	ofn->lStructSize = sizeof(*ofn);
	ofn->hwndOwner = *hwnd;
	ofn->lpstrFile = "\0";
	ofn->nMaxFile = 100;
	ofn->lpstrFilter = (LPCSTR)szFilter;
	ofn->nFilterIndex = 1;
	ofn->lpstrTitle = TEXT("Open");
	ofn->Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
}

//осовобождение памяти
//вход: модели представления
//выход: ...
void FreeData(savedata sData, modeldata mData)
{
	if (sData.strings != NULL)
		free(sData.strings);
	if (mData.strings != NULL)
		free(mData.strings);
	if (mData.stringsWithLayout != NULL)
		free(mData.stringsWithLayout);
}

void FreeMenu(HMENU hmenu)
{
	DeleteMenu(hmenu, IDM_OPEN, MF_BYCOMMAND);
	DeleteMenu(hmenu, IDM_EXIT, MF_BYCOMMAND);
	DeleteMenu(hmenu, IDM_LAYOUT, MF_BYCOMMAND);
	DeleteMenu(hmenu, IDM_DEFAULT, MF_BYCOMMAND);
}

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszArgument, int iCmdShow)
{
	static char szAppName[] = "Reader";
	HWND hwnd;
	MSG msg;
	WNDCLASSEX wndclass;
	HMENU hMenu = LoadMenu(hInstance, MAKEINTRESOURCE(MYMENU));
	setlocale(LC_ALL, "Russian");
	wndclass.cbSize = sizeof(wndclass);
	wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject(ANSI_FIXED_FONT);
	wndclass.lpszMenuName = szAppName;
	wndclass.lpszClassName = szAppName;
	wndclass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	RegisterClassEx(&wndclass);
	hwnd = CreateWindow(
		szAppName,
		"Reader by Natura Anton",
		WS_OVERLAPPEDWINDOW | WS_VSCROLL | WS_HSCROLL,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, hMenu, hInstance, lpszArgument
	);
	SetMenu(hwnd, hMenu);
	ShowWindow(hwnd, iCmdShow);
	UpdateWindow(hwnd);
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	FreeMenu(hMenu);
	//system("pause");
	return msg.wParam;
}
LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	//int* str = (int*)malloc(sizeof(int));
	static int prevLen = 0;
	static OPENFILENAME ofn;
	static int iSelection = IDM_DEFAULT;
	static char nameFile[100] = { 0 };
	static HMENU hMenu;
	static metrixdata metrix;
	static int prevW;
	static int prevH;
	static int flagM = 0;
	HDC hdc;
	PAINTSTRUCT ps;
	static int iPaintBeg, iPaintEnd, iVscrollInc, iHscrollInc;
	TEXTMETRIC tm;
	CREATESTRUCT* pCS;
	char* fileName;
	static int nowScr;
	static savedata sData;
	static modeldata mData;
	static HFONT myFont;
	static int realNum = 0;
	static int prevScroll = 0;
	static int flag = 0;
	static RECT rect;
	switch (iMsg)
	{
	case WM_CREATE:
		pCS = (CREATESTRUCT*)lParam;
		fileName = (char*)pCS->lpCreateParams;
		if (fileName[0] != 0)
			sData = ReadText(fileName);
		else
		{
			mData.maxLen = 0;
			mData.mode = M_DEF;
			sData.size = 0;
			MessageBox(hwnd, TEXT("Press OK and open file with using menu"), TEXT("Error"), MB_OK);
		}
		if (sData.size != 0)
			mData = InitModelData(sData, M_DEF);
		else
			MessageBox(hwnd, TEXT("File isn't read. Press OK and open other file"), TEXT("Error"), MB_OK);
		hdc = GetDC(hwnd);
		myFont = CreateFont(30, 10, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH, "Courier New");
		SelectObject(hdc, myFont);
		GetTextMetrics(hdc, &tm);
		metrix.cxChar = tm.tmAveCharWidth;
		metrix.cyChar = tm.tmHeight + tm.tmExternalLeading;//второй параметр - высота знаков ударения и других
		ReleaseDC(hwnd, hdc);
		metrix.iMaxWidth = mData.maxLen * metrix.cxChar;
		//hMenu = CreateMyMenu(hwnd);
		hMenu = GetMenu(hwnd);
		return 0;
	case WM_SIZE:
		printf("WM_SIZE ");
		if (mData.mode == M_LAY && mData.countOfStrWithLayout != 0 && flag == 0)
		{
			realNum = BinarySearch(mData.countOfStr, mData.strings, mData.stringsWithLayout[metrix.iVscrollPos + 1]);
			flag = 1;
			prevScroll = BinarySearch(mData.countOfStrWithLayout, mData.stringsWithLayout, mData.strings[realNum]);
			realNum--;
		}
		metrix.cxClient = LOWORD(lParam);
		metrix.cyClient = HIWORD(lParam);
		if (metrix.cxChar != 0)
			metrix.lenLayout = metrix.cxClient / metrix.cxChar - 1;
		else
			metrix.lenLayout = 0;
		/*if (metrix.lenLayout >= 1 && mData.countOfStr != 0)
			mData.countOfStrWithLayout = SetLayoutArr(mData.countOfStr, mData.strings, mData.stringsWithLayout, metrix.lenLayout);
		else
			mData.countOfStrWithLayout = 0;*/
		if (metrix.lenLayout >= 1 && mData.countOfStr != 0)
		{
			if (mData.mode == M_LAY && prevLen != metrix.lenLayout)
			{
				mData = SetLayoutArr2(mData, metrix.lenLayout);
				prevLen = metrix.lenLayout;
			}
			if (mData.countOfStrWithLayout != 0)
			{
				prevW = metrix.cxClient;
				prevH = metrix.cyClient;
			//	printf("POS: %d ", metrix.iVscrollPos);
				if (mData.stringsWithLayout != NULL && mData.mode == M_LAY)
                    nowScr = BinarySearch(mData.countOfStr, mData.strings, mData.stringsWithLayout[metrix.iVscrollPos]);
             //   printf("NOWSCR: %d ", nowScr);
				if (flagM == 1)
					mData.countOfStrWithLayout = 0;

			}
			if (prevW == metrix.cxClient && prevH == metrix.cyClient && mData.countOfStrWithLayout == 0 && mData.mode == M_LAY)
			{
				MessageBox(hwnd, TEXT("You can't read text with layout using this and old metrix. After OK application return in default mode"), TEXT("Error"), MB_OK);
				mData.mode = M_DEF;
				flagM = 0;
			}
		}
		else
			mData.countOfStrWithLayout = 0;
		if (mData.countOfStrWithLayout == 0 && mData.mode == M_LAY)
		{
			MessageBox(hwnd, TEXT("You can't read text with layout using this metrix. After OK application return in previously metrix"), TEXT("Error"), MB_OK);
			GetWindowRect(hwnd, &rect);
						flagM = 1;
            metrix.iVscrollPos = nowScr;
			MoveWindow(hwnd, rect.left, rect.top, prevW + 33, prevH + 59, TRUE);
		}
		metrix.iVscrollMax = maximum(0, metrix.cyClient);
		SetScrollRange(hwnd, SB_VERT, 0, metrix.iVscrollMax, FALSE);
		if (mData.mode == M_DEF && mData.countOfStr != 0)
		{
		    //printf("TUT ");
		    //printf("%d %d ", nowScr, metrix.iVscrollPos);
			metrix.iHscrollMax = maximum(0, 2 + (metrix.iMaxWidth - metrix.cxClient) / metrix.cxChar);
			if (mData.countOfStr != 0)
				SetScrollPos(hwnd, SB_VERT, metrix.iVscrollPos * metrix.iVscrollMax / mData.countOfStr, TRUE);
			else
				SetScrollPos(hwnd, SB_VERT, 0, TRUE);
		}
		if (mData.mode == M_LAY && mData.countOfStrWithLayout != 0)
		{
			metrix.iVscrollPos = BinarySearch(mData.countOfStrWithLayout, mData.stringsWithLayout, mData.strings[realNum]);
			SetScrollPos(hwnd, SB_VERT, metrix.iVscrollPos * metrix.iVscrollMax / mData.countOfStrWithLayout, TRUE);
			metrix.iHscrollMax = 0;
		}
		metrix.iHscrollPos = minimum(metrix.iHscrollPos, metrix.iHscrollMax);
		SetScrollRange(hwnd, SB_HORZ, 0, metrix.iHscrollMax, FALSE);
		SetScrollPos(hwnd, SB_HORZ, metrix.iHscrollPos, TRUE);
		return 0;
	case WM_COMMAND:
		printf("WM_COMMAND ");
		switch (LOWORD(wParam))
		{
		case IDM_OPEN:
			InitOFN(&ofn, &hwnd);
			ofn.lpstrFile = nameFile;
			if (GetOpenFileName(&ofn))
			{
				metrix.iHscrollPos = metrix.iVscrollPos = 0;
				FreeData(sData, mData);
				sData = ReadText((char*)ofn.lpstrFile);
				if (sData.size != 0)
				{
					mData = InitModelData(sData, M_DEF);
					metrix.iMaxWidth = mData.maxLen * metrix.cxChar;
					CheckMenuItem(hMenu, iSelection, MF_UNCHECKED);
					iSelection = M_DEF;
				}
				else
					MessageBox(hwnd, TEXT("File isn't read. Press OK and open other file"), TEXT("Error"), MB_OK);
				metrix.iVscrollMax = maximum(0, metrix.cyClient);
				SetScrollRange(hwnd, SB_VERT, 0, metrix.iVscrollMax, FALSE);
				if (mData.countOfStr != 0)
					SetScrollPos(hwnd, SB_VERT, metrix.iVscrollPos* metrix.iVscrollMax / mData.countOfStr, TRUE);
				metrix.iHscrollMax = maximum(0, 2 + (metrix.iMaxWidth - metrix.cxClient) / metrix.cxChar);
				metrix.iHscrollPos = minimum(metrix.iHscrollPos, metrix.iHscrollMax);
				SetScrollRange(hwnd, SB_HORZ, 0, metrix.iHscrollMax, TRUE);
				SetScrollPos(hwnd, SB_HORZ, metrix.iHscrollPos, TRUE);
				metrix.iVscrollPos -= minimum(-1, -metrix.cyClient / metrix.cyChar);
				SendMessage(hwnd, WM_SIZE, metrix.cxClient, metrix.cyClient);
			}
			else
				metrix.iVscrollPos += maximum(1, metrix.cyClient / metrix.cyChar);
			break;
		case IDM_EXIT:
			//printf("IDM_EXIT ");
			SendMessage(hwnd, WM_DESTROY, 0, 0);
		case IDM_DEFAULT:
			CheckMenuItem(hMenu, iSelection, MF_UNCHECKED);
			iSelection = LOWORD(wParam);
			CheckMenuItem(hMenu, iSelection, MF_CHECKED);
			if (mData.mode != M_DEF)
			{
				ShowScrollBar(hwnd, SB_HORZ, TRUE);
				mData.mode = M_DEF;
				flag = 1;
				if (mData.stringsWithLayout != NULL && mData.countOfStr != NULL)
				{
					prevScroll = BinarySearch(mData.countOfStr, mData.strings, mData.stringsWithLayout[metrix.iVscrollPos + 1]);
					free(mData.stringsWithLayout);
					mData.stringsWithLayout = NULL;
				}
				else
					prevScroll = 0;
				metrix.iVscrollPos = prevScroll - 1;
				metrix.iHscrollMax = maximum(0, 2 + (metrix.iMaxWidth - metrix.cxClient) / metrix.cxChar);
				metrix.iHscrollPos = minimum(metrix.iHscrollPos, metrix.iHscrollMax);
				SetScrollRange(hwnd, SB_HORZ, 0, metrix.iHscrollMax, TRUE);
				SetScrollPos(hwnd, SB_HORZ, metrix.iHscrollPos, TRUE);
				mData.stringsWithLayout = NULL;
				InvalidateRect(hwnd, NULL, TRUE);
			}
			break;
		case IDM_LAYOUT:

				if (mData.mode != M_LAY)
				{
					mData = SetLayoutArr2(mData, metrix.lenLayout);
					if (mData.stringsWithLayout != NULL)
					{
						CheckMenuItem(hMenu, iSelection, MF_UNCHECKED);
						iSelection = LOWORD(wParam);
						CheckMenuItem(hMenu, iSelection, MF_CHECKED);
						ShowScrollBar(hwnd, SB_HORZ, FALSE);
						metrix.iHscrollMax = 0;
						metrix.iHscrollPos = 0;
						SetScrollRange(hwnd, SB_HORZ, 0, metrix.iHscrollMax, TRUE);
						SetScrollPos(hwnd, SB_HORZ, metrix.iHscrollPos, TRUE);
						mData.mode = M_LAY;
						metrix.iVscrollPos = BinarySearch(mData.countOfStrWithLayout, mData.stringsWithLayout, mData.strings[metrix.iVscrollPos]);
						flag = 0;
						InvalidateRect(hwnd, NULL, TRUE);
					}
					else
						MessageBox(hwnd, TEXT("Sorry, you can't change mod in this place. Press ok and choose weight"), TEXT("Error"), MB_OK);
				}
			break;
		}
	case WM_KEYDOWN:
		printf("WM_KEYDOWN ");
		switch (wParam)
		{
		case VK_UP:
			metrix.iVscrollPos--;
			InvalidateRect(hwnd, NULL, TRUE);
			break;
		case VK_DOWN:
			metrix.iVscrollPos++;
			InvalidateRect(hwnd, NULL, TRUE);
			break;
		case VK_PRIOR:
			metrix.iVscrollPos += minimum(-1, -metrix.cyClient / metrix.cyChar);
			InvalidateRect(hwnd, NULL, TRUE);
			break;
		case VK_NEXT:
			metrix.iVscrollPos += maximum(1, metrix.cyClient / metrix.cyChar);
			InvalidateRect(hwnd, NULL, TRUE);
			break;
		case VK_LEFT:
			iHscrollInc = -1;
			metrix.iHscrollPos = SetHScroll(iHscrollInc, metrix.iHscrollMax, metrix.iHscrollPos, metrix.cxChar, hwnd);
			break;
		case VK_RIGHT:
			iHscrollInc = 1;
			metrix.iHscrollPos = SetHScroll(iHscrollInc, metrix.iHscrollMax, metrix.iHscrollPos, metrix.cxChar, hwnd);
			break;
		}
	case WM_VSCROLL:
		printf("WM_VSCROLL ");
		flag = 0;
		switch (LOWORD(wParam))
		{
		case SB_LINEUP:
			metrix.iVscrollPos--;
			break;
		case SB_LINEDOWN:
			metrix.iVscrollPos++;
			break;
		case SB_PAGEUP:
			metrix.iVscrollPos += minimum(-1, -metrix.cyClient / metrix.cyChar);
			break;
		case SB_PAGEDOWN:
		    printf("SB_PAGEDOWN ");
			metrix.iVscrollPos += maximum(1, metrix.cyClient / metrix.cyChar);
			break;
		case SB_THUMBTRACK:
		    printf("SB_THUMBTRACK ");
			iVscrollInc = HIWORD(wParam);
			if (mData.mode == M_DEF)
				metrix.iVscrollPos = maximum(((double)iVscrollInc / metrix.iVscrollMax * mData.countOfStr - metrix.cyClient / metrix.cyChar), 0);
			if (mData.mode == M_LAY)
			{
				if (metrix.lenLayout > 1)
				{
					metrix.iVscrollPos = maximum(((double)iVscrollInc / metrix.iVscrollMax * mData.countOfStrWithLayout - metrix.cyClient / metrix.cyChar), 0);
				}
			}
			break;
		default:
			iVscrollInc = 0;
		}
		ScrollWindow(hwnd, 0, -metrix.cyChar * iVscrollInc, NULL, NULL);
		if (mData.mode == M_DEF && mData.countOfStr != 0)
		{
			SetScrollPos(hwnd, SB_VERT, metrix.iVscrollPos * metrix.iVscrollMax / mData.countOfStr, TRUE);
		}
		if (mData.mode == M_LAY && mData.countOfStrWithLayout != 0)
		{
			SetScrollPos(hwnd, SB_VERT, metrix.iVscrollPos * metrix.iVscrollMax / mData.countOfStrWithLayout, TRUE);
		}
		InvalidateRect(hwnd, NULL, TRUE);
		return 0;
	case WM_HSCROLL:
		printf("WM_HSCROLL ");
		switch (LOWORD(wParam))
		{
		case SB_LINEUP:
			iHscrollInc = -1;
			break;
		case SB_LINEDOWN:
			iHscrollInc = 1;
			break;
		case SB_PAGEUP:
			iHscrollInc = -8;
			break;
		case SB_PAGEDOWN:
			iHscrollInc = 8;
			break;
		case SB_THUMBPOSITION:
			iHscrollInc = HIWORD(wParam) - metrix.iHscrollPos;
			break;
		default:
			iHscrollInc = 0;
		}
		metrix.iHscrollPos = SetHScroll(iHscrollInc, metrix.iHscrollMax, metrix.iHscrollPos, metrix.cxChar, hwnd);
		return 0;
	case WM_PAINT:
		printf("WM_PAINT ");
		hdc = BeginPaint(hwnd, &ps);
		iPaintBeg = maximum(0, metrix.iVscrollPos);
		if (mData.mode == M_DEF && sData.size != 0)
		{
			iPaintEnd = minimum(mData.countOfStr, metrix.iVscrollPos + ps.rcPaint.bottom / metrix.cyChar);
			PrintText(iPaintBeg, iPaintEnd, sData.strings, mData.strings, metrix, hdc, mData.mode);
		}
		if (mData.mode == M_LAY && mData.countOfStrWithLayout != 0)
		{
			iPaintEnd = minimum(mData.countOfStrWithLayout, metrix.iVscrollPos + ps.rcPaint.bottom / metrix.cyChar);
			PrintText(iPaintBeg, iPaintEnd, sData.strings, mData.stringsWithLayout, metrix, hdc, mData.mode);
		}
		EndPaint(hwnd, &ps);
		return 0;
	case WM_DESTROY:
		printf("WM_DESTROY ");
		FreeData(sData, mData);
		DeleteObject(myFont);
		mData.countOfStr = 0;
		mData.countOfStrWithLayout = 0;
		metrix.lenLayout = 0;
		PostQuitMessage(0);
		SendMessage(hwnd, WM_QUIT, 0, 0);
		return 0;
	case WM_QUIT:
		printf("WM_QUIT ");
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, iMsg, wParam, lParam);
}
