// leds.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
#include <string.h>
#include <windows.h>
#include "ftd2xx.h"

FT_HANDLE ftHandle; // Handle of the FTDI device
FT_STATUS ftStatus; // Result of each D2XX call
DWORD dwNumDevs; // The number of devices
DWORD dwNumBytesToRead = 0; // Number of bytes available to read in the driver's input buffer
DWORD dwNumBytesRead;
unsigned char byInputBuffer[1024]; // Buffer to hold data read from the FT2232H
DWORD dwNumBytesSent;
DWORD dwNumBytesToSend;
unsigned char byOutputBuffer[1024]; // Buffer to hold MPSSE commands and data to be sent to the FT2232H
int ft232H = 0; // High speed device (FTx232H) found. Default - full speed, i.e. FT2232D
DWORD dwClockDivisor = 0;
DWORD dwCount;

int ftdi_init()
{
FT_DEVICE ftDevice; 
DWORD deviceID; 
char SerialNumber[16+1]; 
char Description[64+1]; 

// Does an FTDI device exist?
printf("Checking for FTDI devices...\n");
ftStatus = FT_CreateDeviceInfoList(&dwNumDevs);

// Get the number of FTDI devices
if (ftStatus != FT_OK) // Did the command execute OK?
{
	printf("Error in getting the number of devices\n");
	return 1; // Exit with error
}

if (dwNumDevs < 1) // Exit if we don't see any
{
	printf("There are no FTDI devices installed\n");
	return 1; // Exist with error
}

printf("%d FTDI devices found - the count includes individual ports on a single chip\n", dwNumDevs);

ftHandle=NULL;

//go thru' list of devices
for(int i=0; i<dwNumDevs; i++)
{
	printf("Open port %d\n",i);
	ftStatus = FT_Open(i, &ftHandle);
	if (ftStatus != FT_OK)
	{
		printf("Open Failed with error %d\n", ftStatus);
		printf("If runing on Linux then try <rmmod ftdi_sio> first\n");
		continue;
	}

	FT_PROGRAM_DATA ftData; 
	char ManufacturerBuf[32]; 
	char ManufacturerIdBuf[16]; 
	char DescriptionBuf[64]; 
	char SerialNumberBuf[16]; 

	ftData.Signature1 = 0x00000000; 
	ftData.Signature2 = 0xffffffff; 
	ftData.Version = 0x00000003;      //3 = FT2232H extensions
	ftData.Manufacturer = ManufacturerBuf; 
	ftData.ManufacturerId = ManufacturerIdBuf; 
	ftData.Description = DescriptionBuf; 
	ftData.SerialNumber = SerialNumberBuf; 
	ftStatus = FT_EE_Read(ftHandle,&ftData);
	if (ftStatus == FT_OK)
	{ 
		printf("\tDevice: %s\n\tSerial: %s\n", ftData.Description, ftData.SerialNumber);
		printf("\tDevice Type: %02X\n", ftData.IFAIsFifo7 );
		break;
	}
	else
	{
		printf("\tCannot read ext flash\n");
	}
}

if(ftHandle==NULL)
{
	printf("NO FTDI chip with FIFO function\n");
	return -1;
}

//ENABLE SYNC FIFO MODE
ftStatus |= FT_SetBitMode(ftHandle, 0xFF, 0x00);
ftStatus |= FT_SetBitMode(ftHandle, 0xFF, 0x40);

if (ftStatus != FT_OK)
{
	printf("Error in initializing1 %d\n", ftStatus);
	FT_Close(ftHandle);
	return 1; // Exit with error
}

UCHAR LatencyTimer = 2; //our default setting is 2
ftStatus |= FT_SetLatencyTimer(ftHandle, LatencyTimer); 
ftStatus |= FT_SetUSBParameters(ftHandle,0x10000,0x10000);
ftStatus |= FT_SetFlowControl(ftHandle,FT_FLOW_RTS_CTS,0x10,0x13);

if (ftStatus != FT_OK)
{
	printf("Error in initializing2 %d\n", ftStatus);
	FT_Close(ftHandle);
	return 1; // Exit with error
}

//return with success
return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
	if( ftdi_init() )
	{
		printf("Cannot init FTDI chip\n");
		return -1;
	}

	byOutputBuffer[0]=0x55;
	FT_Write(ftHandle, byOutputBuffer, 1, &dwNumBytesSent);
	byOutputBuffer[0]=0x33;
	FT_Write(ftHandle, byOutputBuffer, 1, &dwNumBytesSent);
	byOutputBuffer[0]=0xaa;
	FT_Write(ftHandle, byOutputBuffer, 1, &dwNumBytesSent);
	byOutputBuffer[0]=0x69;
	FT_Write(ftHandle, byOutputBuffer, 1, &dwNumBytesSent);

	#define BLOCK_LEN (4096*16)
	unsigned char sbuf[BLOCK_LEN];
	for(int i=0; i<BLOCK_LEN; i++)
		sbuf[i] = (i & 0xff);

		DWORD ticks = GetTickCount();
		int sz = 0;
		while(1)
		{
			FT_Write(ftHandle,sbuf,BLOCK_LEN,&dwNumBytesSent);
			sz += BLOCK_LEN;
			DWORD t = GetTickCount();
			if( (t-ticks) >= 1000)
			{
				//one second lapsed
				printf("sent %d bytes/sec\n",sz);
				ticks = t;
				sz = 0;
			}
		}

	return 0;
}
