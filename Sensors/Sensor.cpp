/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2005-2011 Association Homecinema Francophone.  All rights reserved.
/////////////////////////////////////////////////////////////////////////////
//
//  This file is subject to the terms of the GNU General Public License as
//  published by the Free Software Foundation.  A copy of this license is
//  included with this software distribution in the file COPYING.htm. If you
//  do not have a copy, you may obtain a copy by writing to the Free
//  Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
//
//  This software is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details
/////////////////////////////////////////////////////////////////////////////
//  Author(s):
//	François-Xavier CHABOUD
/////////////////////////////////////////////////////////////////////////////

// Sensor.cpp: implementation of the CSensor class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ColorHCFR.h"
#include "Sensor.h"
#include "Generator.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_SERIAL(CSensor, CObject, 1) ;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSensor::CSensor()
{
	m_isModified=FALSE;
	m_isMeasureValid=TRUE;
	m_sensorToXYZMatrix=IdentityMatrix(3);
	m_sensorToXYZMatrixMod=IdentityMatrix(3);

	m_calibrationTime=0;

	m_PropertySheetTitle = IDS_SENSOR_PROPERTIES_TITLE;

	m_pDevicePage = NULL;
	m_pCalibrationPage = NULL;

	SetName("Not defined");  // Needs to be set for real sensors
}

CSensor::~CSensor()
{

}

void CSensor::Copy(CSensor * p)
{
	m_errorString = p->m_errorString;
	m_isMeasureValid = p->m_isMeasureValid;
	m_sensorToXYZMatrix = p->m_sensorToXYZMatrix;
	m_calibrationTime = p->m_calibrationTime;
	m_name = p->m_name;
}

void CSensor::Serialize(CArchive& archive)
{
	CObject::Serialize(archive);
	m_sensorToXYZMatrix.Serialize(archive); 
	if (archive.IsStoring())
	{
		int version=1;
		archive << version;
		// Stored as 32 bits, as the 32-bit build (_USE_32BIT_TIME_T) always did:
		// time_t is 64-bit on x64 and must not change the .chc file format
		archive << (LONG) m_calibrationTime;
	}
	else
	{
		int version;
		archive >> version;
		if ( version > 1 )
			AfxThrowArchiveException ( CArchiveException::badSchema );
		LONG calibrationTime32;
		archive >> calibrationTime32;
		m_calibrationTime = (time_t) calibrationTime32;
	}
}

void CSensor::SetPropertiesSheetValues()
{
	m_SensorPropertiesPage.SetMatrix(m_sensorToXYZMatrix);
	m_SensorPropertiesPage.m_calibrationDate=GetCalibrationTime().Format("%#c");
}

void CSensor::GetPropertiesSheetValues()
{
	if(	m_sensorToXYZMatrix != m_SensorPropertiesPage.GetMatrix() )
	{
		m_sensorToXYZMatrix=m_SensorPropertiesPage.GetMatrix();
		SetModifiedFlag(TRUE);
	}
}

BOOL CSensor::Init( BOOL bForSimultaneousMeasures )
{
	return TRUE;
}

BOOL CSensor::Release()
{
	
	return TRUE;
}

CColor CSensor::MeasureColor(const ColorRGBDisplay& aRGBValue, int displaymode)
{
	CColor result;
	SetMeasureValidity(TRUE);	// MeasureColorInternal may report a failure with its own message
	if (this->GetName() == "Simulated sensor")
	    result = MeasureColorInternal(aRGBValue, displaymode);
	else
		result = MeasureColorInternal(aRGBValue);
	
	// a failed reading stays invalid (it was clamped to a valid near-zero colour before, and
	// stored as a black measure with no error shown): the measure loops ask Abort/Retry/Ignore
	if (!result.isValid())
	{
		if (IsMeasureValid())
			SetErrorString("The meter reading failed (meter saturated, disconnected or not calibrated).");
		SetMeasureValidity(FALSE);
		return result;
	}

	result.SetX(max(result.GetX(),0.00000001));
	result.SetY(max(result.GetY(),0.00000001));
	result.SetZ(max(result.GetZ(),0.00000001));
    result.applyAdjustmentMatrix(m_sensorToXYZMatrix);
    return result;
}

CColor CSensor::MeasureGray(double aLevel)
{
	// by default use pure virtual DisplayRGBColor function
	return MeasureColor(ColorRGBDisplay(aLevel)); 
}

BOOL CSensor::Configure()
{
	CString	str;
	CPropertySheetWithHelp propertySheet;

	str.LoadString(m_PropertySheetTitle);
	propertySheet.SetTitle(str);
	propertySheet.m_psh.dwFlags |= PSH_NOAPPLYNOW;

	if ( m_pDevicePage )
		propertySheet.AddPage ( m_pDevicePage );

	propertySheet.AddPage ( & m_SensorPropertiesPage );
	propertySheet.SetActivePage(0);
	SetPropertiesSheetValues();
	int result=propertySheet.DoModal();
	if(result == IDOK)
		GetPropertiesSheetValues();

	return result==IDOK;
}
