#include "WindowsApplication.h"
#include <Commdlg.h>

WindowsApplication::WindowsApplication() :
	m_hWnd(NULL),
	m_nStartTime(0),
	m_nLastCounter(0),
	m_nFramesSinceUpdate(0),
	m_fFreq(0),
	m_nNextStatusTime(0),
	m_isCloudWritingStarted(false),
	m_recordingConfiguration(new std::vector<RecordingConfiguration>())
{
	LARGE_INTEGER qpf = { 0 };
	if (QueryPerformanceFrequency(&qpf))
	{
		m_fFreq = double(qpf.QuadPart);
	}
	initRecordDataModel();
}

void WindowsApplication::initRecordDataModel()
{
	for (int i = 0; i < RECORD_CLOUD_TYPE_COUNT; i++){
		m_recordingConfiguration->emplace_back(static_cast<RecordCloudType>(i), PLY);
	}
	
	
	m_recordingConfiguration->at(	HDFace	 ).recordConfigurationStatusChanged.connect(boost::bind(&RecordTabHandler::recordConfigurationStatusChanged, &m_recordTabHandler, _1, _2));
	m_recordingConfiguration->at(	FaceRaw	 ).recordConfigurationStatusChanged.connect(boost::bind(&RecordTabHandler::recordConfigurationStatusChanged, &m_recordTabHandler, _1, _2));
	m_recordingConfiguration->at(FullDepthRaw).recordConfigurationStatusChanged.connect(boost::bind(&RecordTabHandler::recordConfigurationStatusChanged, &m_recordTabHandler, _1, _2));
	

	m_recordingConfiguration->at(	HDFace	 ).recordPathOrFileNameChanged.connect(boost::bind(&RecordTabHandler::recordPathChanged, &m_recordTabHandler, _1));
	m_recordingConfiguration->at(	FaceRaw	 ).recordPathOrFileNameChanged.connect(boost::bind(&RecordTabHandler::recordPathChanged, &m_recordTabHandler, _1));	
	m_recordingConfiguration->at(FullDepthRaw).recordPathOrFileNameChanged.connect(boost::bind(&RecordTabHandler::recordPathChanged, &m_recordTabHandler, _1));

	
	

	
}


WindowsApplication::~WindowsApplication()
{
	SafeRelease(m_pD2DFactory);
}




/// <summary>
/// Creates the main window and begins processing
/// </summary>
/// <param name="hInstance">handle to the application instance</param>
/// <param name="nCmdShow">whether to display minimized, maximized, or normally</param>
int WindowsApplication::run(HINSTANCE hInstance, int nCmdShow)
{

	MSG       msg = { 0 };
	WNDCLASS  wc;

	// Dialog custom window class
	ZeroMemory(&wc, sizeof(wc));
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
	wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCE(IDI_APP));
	wc.lpfnWndProc = DefDlgProcW;
	wc.lpszClassName = L"KinectHDFaceGrabberAppDlgWndClass";
	m_hInstance = hInstance;
	if (!RegisterClassW(&wc))
	{
		return 0;
	}

	// Create main application window
	HWND hWndApp = CreateDialogParamW(
		NULL,
		MAKEINTRESOURCE(IDD_APP_TAB),
		NULL,
		(DLGPROC)WindowsApplication::MessageRouter,
		reinterpret_cast<LPARAM>(this));

	// Show window
	ShowWindow(hWndApp, nCmdShow);

	// Main message loop
	while (WM_QUIT != msg.message)
	{
		m_kinectFrameGrabber.update();
		//m_kinectDepthGrabber.update();
		while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
		{
			// If a dialog message will be taken care of by the dialog proc
			if (hWndApp && IsDialogMessageW(hWndApp, &msg))
			{
				continue;
			}

			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
		//m_cloudViewer.wasStopped();
	}

	return static_cast<int>(msg.wParam);
}


/// <summary>
/// Handles window messages, passes most to the class instance to handle
/// </summary>
/// <param name="hWnd">window message is for</param>
/// <param name="uMsg">message</param>
/// <param name="wParam">message data</param>
/// <param name="lParam">additional message data</param>
/// <returns>result of message processing</returns>
LRESULT CALLBACK WindowsApplication::MessageRouter(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	WindowsApplication* pThis = nullptr;

	if (WM_INITDIALOG == uMsg)
	{
		pThis = reinterpret_cast<WindowsApplication*>(lParam);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
	}
	else
	{
		pThis = reinterpret_cast<WindowsApplication*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
	}

	if (pThis)
	{
		return pThis->DlgProc(hWnd, uMsg, wParam, lParam);
	}

	return 0;
}

void WindowsApplication::imageUpdated(const unsigned char *data, unsigned width, unsigned height)
{
	OutputDebugString(L"Imageupdate");
	//this->m_imageViewer->showRGBImage(data, width, height);
}
void WindowsApplication::cloudUpdate(pcl::PointCloud<pcl::PointXYZRGB>::ConstPtr cloud)
{
	//m_cloudViewer.showCloud(cloud);
}

#include <windowsx.h>

int InsertTabItem(HWND hTab, LPTSTR pszText, int iid)
{
	TCITEM ti = { 0 };
	ti.mask = TCIF_TEXT;
	ti.pszText = pszText;
	ti.cchTextMax = wcslen(pszText);
	return TabCtrl_InsertItem(hTab, iid, &ti);
}


LRESULT CALLBACK WindowsApplication::DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);

	switch (message)
	{
	case WM_INITDIALOG:
	{
		// Bind application window handle
		m_hWnd = hWnd;

		// Init Direct2D
		D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);

		// Create and initialize a new Direct2D image renderer (take a look at ImageRenderer.h)
		// We'll use this to draw the data we receive from the Kinect to the screen
		m_pDrawDataStreams = new ImageRenderer();
		m_pclFaceViewer = std::shared_ptr<PCLViewer>(new PCLViewer(2, "Face-Viewer"));
		//m_pclFaceRawViewer = std::shared_ptr<PCLViewer>(new PCLViewer("Raw Face-Depth"));

		m_recordTabHandler.setSharedRecordingConfiguration(m_recordingConfiguration);
		m_recordTabHandle = CreateDialogParamW(
			NULL,
			MAKEINTRESOURCE(IDC_TAB_RECORD),
			m_hWnd,
			(DLGPROC)RecordTabHandler::MessageRouterTab,
			reinterpret_cast<LPARAM>(&m_recordTabHandler));

		m_plackBackTabHandler.setSharedRecordingConfiguration(m_recordingConfiguration);

		m_playbackTabHandle = CreateDialogParamW(
			NULL,
			MAKEINTRESOURCE(IDC_TAB_PLAYBACK),
			m_hWnd,
			(DLGPROC)PlaybackTabHandler::MessageRouterTab,
			reinterpret_cast<LPARAM>(&m_plackBackTabHandler));
		ShowWindow(m_playbackTabHandle, SW_HIDE);
		//HRESULT hr = m_pDrawDataStreams->initialize(GetDlgItem(m_hWnd, IDC_TAB2), m_pD2DFactory, cColorWidth, cColorHeight, cColorWidth * sizeof(RGBQUAD));
		
		/*
		TCITEM tab2Data;
		tab2Data.mask = TCIF_TEXT;
		tab2Data.pszText = L"Tab2";

		TabCtrl_InsertItem(m_hWnd, 1, &tab2Data);*/
		
		RECT windowRect;

		GetClientRect(m_hWnd, &windowRect);
		HWND tabControlHandle = GetDlgItem(m_hWnd, IDC_TAB2);
		
		InsertTabItem(tabControlHandle, L"Record", 0);
		InsertTabItem(tabControlHandle, L"Playback", 1);
		
		//TabCtrl_InsertItem(m_hWnd, 0, &tab1Data);
		
		TabCtrl_SetCurSel(tabControlHandle, 0);
		ShowWindow(tabControlHandle, SW_SHOW);

		RECT tabControlRect;
		GetWindowRect(tabControlHandle, &tabControlRect);
		
		/*

		m_listView.OnCreate(m_hWnd, reinterpret_cast<CREATESTRUCT*>(lParam),
			tabControlRect.left, tabControlRect.bottom, (tabControlRect.right - tabControlRect.left) / 2,
			(windowRect.bottom - tabControlRect.bottom - 180) / 2);
*/
		

		//Edit_SetText(GetDlgItem(m_hWnd, IDC_HDFACE_EDIT_BOX), m_recordingConfiguration[HDFace].getFileName());
		//Edit_SetText(GetDlgItem(m_hWnd, IDC_FACE_RAW_EDIT_BOX), m_recordingConfiguration[FaceRaw].getFileName());
		//Edit_SetText(GetDlgItem(m_hWnd, IDC_FULL_RAW_DEPTH_EDIT_BOX), m_recordingConfiguration[FullDepthRaw].getFileName());

		//HWND hdFaceComboBox			= GetDlgItem(m_hWnd, IDC_HD_FACE_COMBO_BOX);
		//HWND facerawDepthComboBox	= GetDlgItem(m_hWnd, IDC_FACE_RAW_DEPTH_COMBO_BOX);
		//HWND fullRawDepthCombobox	= GetDlgItem(m_hWnd, IDC_FULL_RAW_DEPTH_COMBO_BOX);

		////for (int i = RECORD_FILE_FORMAT_COUNT-1; i >= 0; --i){
		//for (int i = 0; i < RECORD_FILE_FORMAT_COUNT; i++){
		//	LPTSTR fileFormatName = RecordingConfiguration::getFileFormatAsString(static_cast<RecordingFileFormat>(i));
		//	ComboBox_AddString(hdFaceComboBox,		 fileFormatName);
		//	ComboBox_AddString(facerawDepthComboBox, fileFormatName);
		//	ComboBox_AddString(fullRawDepthCombobox, fileFormatName);
		//	if (i == 0){
		//		ComboBox_SetCurSel(hdFaceComboBox,			i);
		//		ComboBox_SetCurSel(facerawDepthComboBox,	i);
		//		ComboBox_SetCurSel(fullRawDepthCombobox,	i);
		//	}
		//}

		// Load and register Listview control class
		
		//HWND m_liveViewWindow = CreateDialog(m_hInstance, MAKEINTRESOURCE(IDC_TAB_1), m_tabHandle, 0); // Setting dialog to tab one by default
		//HRESULT hr = m_pDrawDataStreams->initialize(tab1Handle, m_pD2DFactory, cColorWidth, cColorHeight, cColorWidth * sizeof(RGBQUAD));
		//InsertTabItem(tab, L"myItem", 0);
		const int width = 869;
		const int height = 489;
		const int xPos = (windowRect.right - windowRect.left - width)/2;
		m_liveViewWindow = CreateWindow(WC_STATIC, L"", WS_CHILD | WS_VISIBLE, xPos, tabControlRect.top, width, height, m_hWnd, NULL, m_hInstance, NULL);
		
		RECT liveViewRect;
		GetWindowRect(m_liveViewWindow, &liveViewRect);
		
		m_cloudOutputWriter = std::shared_ptr<KinectCloudOutputWriter>(new KinectCloudOutputWriter);
		HRESULT hr = m_pDrawDataStreams->initialize(m_liveViewWindow, m_pD2DFactory, cColorWidth, cColorHeight, cColorWidth * sizeof(RGBQUAD));

		if (FAILED(hr))
		{
			setStatusMessage(L"Failed to initialize the Direct2D draw device.", true);
		}
		
		m_kinectFrameGrabber.setImageRenderer(m_pDrawDataStreams);
		
		// Get and initialize the default Kinect sensor
		m_kinectFrameGrabber.initializeDefaultSensor();
		//m_kinectFrameGrabber.depthCloudUpdated.connect(boost::bind(&WindowsApplication::cloudUpdate, this, _1));		

		m_kinectFrameGrabber.cloudUpdated.connect(boost::bind(&PCLViewer::updateCloudThreated, m_pclFaceViewer, _1, 0));
		m_kinectFrameGrabber.depthCloudUpdated.connect(boost::bind(&PCLViewer::updateCloudThreated, m_pclFaceViewer, _1, 1));

		m_kinectFrameGrabber.depthCloudUpdated.connect(boost::bind(&KinectCloudOutputWriter::updateCloudThreated, m_cloudOutputWriter, _1));
		//m_kinectFrameGrabber.cloudUpdated.connect(boost::bind(&KinectCloudOutputWriter::updateCloudThreated, m_cloudOutputWriter, _1));

		m_kinectFrameGrabber.statusChanged.connect(boost::bind(&WindowsApplication::setStatusMessage, this, _1, _2));
		
		
		
	}
		break;

		// If the titlebar X is clicked, destroy app
	case WM_CLOSE:
		DestroyWindow(hWnd);
		//m_pclFaceRawViewer->stopViewer();
		m_pclFaceViewer->stopViewer();
		break;

	case WM_DESTROY:
		// Quit the main message pump
		//m_listView.OnDestroy(m_hWnd);
		PostQuitMessage(0);
		break;
	case WM_COMMAND:
		processUIMessage(wParam, lParam);
		break;
	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code)
		{
		case TCN_SELCHANGE:
			int iPage = TabCtrl_GetCurSel(GetDlgItem(m_hWnd, IDC_TAB2));
			if (iPage == 0){
				onRecordTabSelected();
			}
			else{
				onPlaybackSelected();
			}
			break;
		}
	
		default:
			break;
		
		break;

	}
	return FALSE;
}

void WindowsApplication::onRecordTabSelected()
{
	//m_listView.OnHide();
	ShowWindow(m_liveViewWindow, SW_SHOW);
	ShowWindow(m_recordTabHandle, SW_SHOW);
	ShowWindow(m_playbackTabHandle, SW_HIDE);
}
void WindowsApplication::onPlaybackSelected()
{
	//m_listView.OnShow();
	ShowWindow(m_liveViewWindow, SW_HIDE);
	ShowWindow(m_recordTabHandle, SW_HIDE);
	ShowWindow(m_playbackTabHandle, SW_SHOW);
}


void WindowsApplication::onSelectionChanged(WPARAM wParam, LPARAM handle)
{
	
	switch (LOWORD(wParam))
	{
	default:
		break;
	}
}
void WindowsApplication::onEditBoxeChanged(WPARAM wParam, LPARAM handle)
{
	
	switch (LOWORD(wParam))
	{
	default:
		break;
	}
}

void WindowsApplication::onButtonClicked(WPARAM wParam, LPARAM handle)
{
	switch (LOWORD(wParam))
	{
	default:
		break;
	}
}
void WindowsApplication::processUIMessage(WPARAM wParam, LPARAM handle)
{

	switch (HIWORD(wParam))
	{
	case CBN_SELCHANGE:
		onSelectionChanged(wParam, handle);
		break;
	case BN_CLICKED:
		onButtonClicked(wParam, handle);
		break;
	case EN_CHANGE:
		onEditBoxeChanged(wParam, handle);
	default:
		break;
	}	
}

bool WindowsApplication::setStatusMessage(std::wstring statusString, bool bForce)
{
	_In_z_ WCHAR* szMessage = &statusString[0];
	ULONGLONG now = GetTickCount64();
	ULONGLONG nShowTimeMsec = 1000;
	if (m_hWnd && (bForce || (m_nNextStatusTime <= now)))
	{
		SetDlgItemText(m_hWnd, IDC_STATUS, szMessage);
		m_nNextStatusTime = now + nShowTimeMsec;

		return true;
	}

	return false;
}

void WindowsApplication::recordPathChanged(RecordCloudType type)
{

}

void WindowsApplication::recordConfigurationStatusChanged(RecordCloudType type, bool newState)
{
	
}




bool WindowsApplication::openFileDialog(WCHAR* szDir, HWND handle)
{
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));

	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = handle;
	ofn.lpstrFile = szDir;

	// Set lpstrFile[0] to '\0' so that GetOpenFileName does not
	// use the contents of szFile to initialize itself.
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = L"ply\0*.ply\0pcd\0*.pcd\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn))
	{
		// Do something usefull with the filename stored in szFileName 
		return true;
	}
	return false;
}

bool WindowsApplication::openDirectoryDialog(WCHAR* szDir, HWND handle)
{
	BROWSEINFO bInfo;
	bInfo.hwndOwner = handle;
	bInfo.pidlRoot = NULL;
	bInfo.pszDisplayName = szDir; // Address of a buffer to receive the display name of the folder selected by the user
	bInfo.lpszTitle = L"Please, select a output folder"; // Title of the dialog
	bInfo.ulFlags = BIF_USENEWUI;
	bInfo.lpfn = NULL;
	bInfo.lParam = 0;
	bInfo.iImage = -1;

	LPITEMIDLIST lpItem = SHBrowseForFolder(&bInfo);
	if (lpItem != NULL)
	{
		if (SHGetPathFromIDList(lpItem, szDir)){
			return true;
		}
	}
	return false;
}