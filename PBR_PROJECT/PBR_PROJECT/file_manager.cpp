#include "file_manager.h"

#ifdef WIN32
#include <ShObjIdl.h>
#else

#endif

std::filesystem::path GetPathFromOpenDialog() {
	std::filesystem::path ret_path;
#ifdef WIN32
	IFileOpenDialog* pFileOpen;
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
		IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

	if (SUCCEEDED(hr)) {
		pFileOpen->Show(NULL);
		IShellItem* p_item;
		hr = pFileOpen->GetResult(&p_item);
		if (SUCCEEDED(hr)) {
			PWSTR pszFilePath;
			hr = p_item->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
			if (SUCCEEDED(hr)) {
				ret_path = pszFilePath;
			}
			p_item->Release();
		}
	}
	pFileOpen->Release();
#else
	static_assert(false && "No Implementation");
#endif // WIN32		
	return ret_path;
}