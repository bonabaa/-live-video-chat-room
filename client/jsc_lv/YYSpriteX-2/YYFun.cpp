#include <ptlib.h>
#include <stdio.h>
#include <windows.h>
 #include <shlobj.h>
extern "C" 
{ 
#include "../winvnc/libjpeg/jpeglib.h"
} 
bool gfx_DeleteDirectory(const PString& strPath)
{
	if (PDirectory::Exists(strPath) )
	{
		PDirectory dir(strPath);
		dir.Open();
		
		do 
		{
			PString name(PString::Printf, "%s/%s", (const char*)strPath, (const char*)dir.GetEntryName() )  ;
			if (dir.GetEntryName().GetLength() <=0)
				break;
			PFileInfo  fi ;
			PFile::GetInfo(name,fi);
			if (fi.type ==  PFileInfo::SubDirectory )
			//if (!PFile::Remove(name) )
			{
				gfx_DeleteDirectory(name);
			}
			else
			{
				PFile::Remove(name);
			}

		}while (dir.Next());

		dir.Close();
#ifdef _WIN32
		PDirectory::Remove(strPath );
#else
		if (strPath.Right(1) != "/")
			PDirectory::Remove(strPath +"/");
#endif

	}
	else
		return true;

	return true;
}

char* GetAppPath()
{
  static bool bInit=false;
	static char MyDir[512]; 
  if (bInit == false){
    
  SHGetSpecialFolderPath(0,MyDir,CSIDL_PERSONAL,0); 
  bInit= true;
  //SHGetSpecialFolderPath(0,MyDir,CSIDL_COMMON_DOCUMENTS,0); 
  }
  return MyDir;

}
char* GetAppPath2()
{
	static char szBuff[_MAX_PATH];
	::GetModuleFileName(NULL, szBuff, _MAX_PATH);
	char* pszFind = strrchr(szBuff, '\\');
	if(pszFind!=0 && pszFind != szBuff) {
		//*(pszFind-1) = '\0';
		*pszFind = '\0';
	}
	return szBuff;
}int SaveCaptureTojpegFile(const char* filename,HDC hDCmem,int width, int height, int quality)
{
//HBITMAP hBMP;
 
struct jpeg_compress_struct cinfo;
struct jpeg_error_mgr jerr;
FILE * outfile;
JSAMPLE* scanline;
COLORREF pixel;

 

cinfo.err = jpeg_std_error(&jerr);
jpeg_create_compress(&cinfo);
outfile = fopen(filename, "wb");
if(outfile == NULL) return -1;
jpeg_stdio_dest(&cinfo, outfile);
cinfo.image_width = width;
cinfo.image_height = height;
cinfo.input_components = 3;
cinfo.in_color_space = JCS_RGB;
jpeg_set_defaults(&cinfo);
if(quality < 0) quality = 0;
if(quality > 100) quality = 100;
jpeg_set_quality(&cinfo, quality, FALSE);
jpeg_start_compress(&cinfo, TRUE);
scanline = new JSAMPLE[width*3];
for(int posy = 0; posy < height; posy++) 
{
for(int posx = 0; posx < width; posx++)
{
pixel = GetPixel(hDCmem, posx, posy);
scanline[posx*3+0] = GetRValue(pixel);
scanline[posx*3+1] = GetGValue(pixel);
scanline[posx*3+2] = GetBValue(pixel);
}
jpeg_write_scanlines(&cinfo, &scanline, 1);
}
jpeg_finish_compress(&cinfo);

jpeg_destroy_compress(&cinfo);
delete scanline;
fclose(outfile);
 return 0;
}

int jpegCapture(char* filename, int quality)
{
HBITMAP hBMP;
HWND desktopWnd;
int width;
int height;
RECT rc;
HDC hDC;
HDC hDCmem;

struct jpeg_compress_struct cinfo;
struct jpeg_error_mgr jerr;
FILE * outfile;
JSAMPLE* scanline;
COLORREF pixel;

desktopWnd = GetDesktopWindow();
GetWindowRect(desktopWnd, &rc);
width = rc.right - rc.left;
height = rc.bottom - rc.top;
hDC = GetDC(desktopWnd);
hDCmem = CreateCompatibleDC(hDC);
hBMP = CreateCompatibleBitmap(hDC, width, height);
if(hBMP == NULL) return -2;
SelectObject(hDCmem, hBMP);
BitBlt(hDCmem, 0, 0, width, height, hDC, rc.left, rc.top, SRCCOPY);

cinfo.err = jpeg_std_error(&jerr);
jpeg_create_compress(&cinfo);
outfile = fopen(filename, "wb");
if(outfile == NULL) return -1;
jpeg_stdio_dest(&cinfo, outfile);
cinfo.image_width = width;
cinfo.image_height = height;
cinfo.input_components = 3;
cinfo.in_color_space = JCS_RGB;
jpeg_set_defaults(&cinfo);
if(quality < 0) quality = 0;
if(quality > 100) quality = 100;
jpeg_set_quality(&cinfo, quality, FALSE);
jpeg_start_compress(&cinfo, TRUE);
scanline = new JSAMPLE[width*3];
for(int posy = 0; posy < height; posy++) 
{
for(int posx = 0; posx < width; posx++)
{
pixel = GetPixel(hDCmem, posx, posy);
scanline[posx*3+0] = GetRValue(pixel);
scanline[posx*3+1] = GetGValue(pixel);
scanline[posx*3+2] = GetBValue(pixel);
}
jpeg_write_scanlines(&cinfo, &scanline, 1);
}
jpeg_finish_compress(&cinfo);

jpeg_destroy_compress(&cinfo);
delete scanline;
fclose(outfile);
DeleteDC(hDCmem);
ReleaseDC(desktopWnd, hDC);
return 0;
}
