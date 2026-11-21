// ColorHCFR Device Interface DLL
// This DLL is NOT under GNU License, and its source code is private.
// This is the needed interface for Eye One sensor.


#ifdef CHCFRDI2_EXPORTS
#define CHCFRDI2_API __declspec(dllexport)
#else
#define CHCFRDI2_API __declspec(dllimport)
#endif

#ifdef __cplusplus
	extern "C" {
#endif

#define DI2ERR_SUCCESS		0
#define DI2ERR_NODLL		1
#define DI2ERR_INVALIDDLL	2
#define DI2ERR_EYEONE		3
#define DI2ERR_NOTCONNECTED	4
#define DI2ERR_UNKNOWN		5

#if defined(_WIN64) && !defined(CHCFRDI2_EXPORTS)
// 64-bit build: this closed-source device DLL exists only as a 32-bit binary,
// so the native driver reports "DLL not available" (use the Argyll driver instead)
static inline int InitDevice2(UINT, BOOL, LPSTR lpszErrorMsg) { if (lpszErrorMsg) lstrcpyA(lpszErrorMsg, "Eye One native driver is not available in the 64-bit build, use the Argyll driver"); return DI2ERR_NODLL; }
static inline BOOL ReleaseDevice2() { return FALSE; }
static inline BOOL GetValuesDevice2(double *, double *, double *, double *, BOOL * pbSpectrumOk) { if (pbSpectrumOk) *pbSpectrumOk = FALSE; return FALSE; }
static inline BOOL CalibrateDevice2(LPSTR) { return FALSE; }
static inline BOOL Device2NeedCalibration() { return FALSE; }
#else
CHCFRDI2_API int InitDevice2(UINT nCalibrationMode, BOOL bAmbientLight, LPSTR lpszErrorMsg);
CHCFRDI2_API BOOL ReleaseDevice2();
CHCFRDI2_API BOOL GetValuesDevice2(double * pX, double * pY, double * pZ, double * pSpectrum, BOOL * pbSpectrumOk);
CHCFRDI2_API BOOL CalibrateDevice2(LPSTR lpszErrorMsg);
CHCFRDI2_API BOOL Device2NeedCalibration();
#endif

#ifdef __cplusplus
	} // extern "C"
#endif

