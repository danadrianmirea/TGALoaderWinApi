#include <windows.h>
#include <fstream>
#include <vector>

#pragma pack(push, 1)
struct TGAHeader {
    BYTE idLength;
    BYTE colorMapType;
    BYTE imageType;
    WORD colorMapOrigin;
    WORD colorMapLength;
    BYTE colorMapDepth;
    WORD xOrigin;
    WORD yOrigin;
    WORD width;
    WORD height;
    BYTE pixelDepth;
    BYTE imageDescriptor;
};
#pragma pack(pop)

struct TGAImage {
    int width;
    int height;
    std::vector<BYTE> pixels; // RGBA format
};

// Load a TGA file into memory
bool LoadTGA(const char* filename, TGAImage& image) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) return false;

    TGAHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(TGAHeader));

    if (header.imageType != 2 || (header.pixelDepth != 24 && header.pixelDepth != 32)) {
        // Only support uncompressed RGB/RGBA images
        return false;
    }

    image.width = header.width;
    image.height = header.height;
    int pixelSize = header.pixelDepth / 8;
    int imageSize = header.width * header.height * 4; // RGBA 32-bit output

    image.pixels.resize(imageSize);
    std::vector<BYTE> tempPixels(header.width * header.height * pixelSize);
    file.read(reinterpret_cast<char*>(tempPixels.data()), tempPixels.size());

    for (int i = 0; i < header.width * header.height; ++i) {
        image.pixels[i * 4 + 0] = tempPixels[i * pixelSize + 2]; // R
        image.pixels[i * 4 + 1] = tempPixels[i * pixelSize + 0]; // G
        image.pixels[i * 4 + 2] = tempPixels[i * pixelSize + 1]; // B
        image.pixels[i * 4 + 3] = (pixelSize == 4) ? tempPixels[i * pixelSize + 3] : 255; // A
    }
    return true;
}

// Display the TGA image using Win32
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static TGAImage image;
    static HDC hdcMem = nullptr;
    static HBITMAP hBitmap = nullptr;

    switch (uMsg) {
    case WM_CREATE: {
        if (!LoadTGA("testimage.tga", image)) {
            MessageBox(hwnd, "Failed to load TGA file", "Error", MB_OK | MB_ICONERROR);
            PostQuitMessage(0);
            return 0;
        }

        HDC hdc = GetDC(hwnd);
        hdcMem = CreateCompatibleDC(hdc);

        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = image.width;
        bmi.bmiHeader.biHeight = -image.height; // Negative to flip vertically
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32; // 32-bit color
        bmi.bmiHeader.biCompression = BI_RGB;

        // Create DIB Section and link directly to the image pixel data
        void* bitmapData = nullptr;
        hBitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &bitmapData, nullptr, 0);

        // Copy pixels to the DIB section if needed
        if (bitmapData) {
            memcpy(bitmapData, image.pixels.data(), image.pixels.size());
        }

        SelectObject(hdcMem, hBitmap);
        ReleaseDC(hwnd, hdc);
    } break;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        BitBlt(hdc, 0, 0, image.width, image.height, hdcMem, 0, 0, SRCCOPY);
        EndPaint(hwnd, &ps);
    } break;

    case WM_DESTROY: {
        if (hBitmap) DeleteObject(hBitmap);
        if (hdcMem) DeleteDC(hdcMem);
        PostQuitMessage(0);
    } break;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

// Win32 entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    const char CLASS_NAME[] = "TGAWindowClass";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, CLASS_NAME, "TGA Viewer", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        nullptr, nullptr, hInstance, nullptr);

    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
