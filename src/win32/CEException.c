/* ScummVM - Scumm Interpreter
 * Copyright (C) 2001-2006 The ScummVM project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $Header: /cvsroot/roadmap/roadmap/src/win32/CEException.c,v 1.1 2009/02/09 19:39:17 dannybackx Exp $
 *
 */

#include "CEException.h"

void ce_exception_writestring(HANDLE file, char *data)
{
	DWORD dummy;
	WriteFile(file, data, strlen(data), &dummy, NULL);
	WriteFile(file, "\r\n", 2, &dummy, NULL);
}

void ce_exception_writeBreak(HANDLE file)
{
	char tempo[100];
	int i;

	memset(tempo, 0, sizeof(tempo));
	for (i=0; i<40; i++)
		tempo[i] = '-';
	ce_exception_writestring(file, tempo);
}

void ce_exception_dumpContext(HANDLE file, HANDLE hProcess, CONTEXT *context)
{
	char tempo[200];
	unsigned char memoryDump[100];
	DWORD size;
	unsigned int i;

#ifdef ARM
	ce_exception_writeBreak(file);
	ce_exception_writestring(file, (char *)"Context dump");
	sprintf(tempo, "R0=%.8lx R1=%.8lx R2=%.8lx R3=%.8lx R4=%.8lx", context->R0, context->R1,
		context->R2, context->R3, context->R4);
	ce_exception_writestring(file, tempo);
	sprintf(tempo, "R5=%.8lx R6=%.8lx R7=%.8lx R8=%.8lx R9=%.8lx", context->R5, context->R6,
		context->R7, context->R8, context->R9);
	ce_exception_writestring(file, tempo);
	sprintf(tempo, "R10=%.8lx R11=%.8lx R12=%.8lx", context->R10, context->R11,
		context->R12);
	ce_exception_writestring(file, tempo);
	sprintf(tempo, "Sp=%.8lx Lr=%.8lx Pc=%.8lx Psr=%.8lx", context->Sp, context->Lr,
		context->Pc, context->Psr);
	ce_exception_writestring(file, tempo);
	ce_exception_writeBreak(file);

	sprintf(tempo, "Memory dump at %.8lx", context->Pc - (sizeof(memoryDump) / 2));
	ce_exception_writestring(file, tempo);
	if (ReadProcessMemory(hProcess, (LPCVOID)(context->Pc - (sizeof(memoryDump) / 2)), memoryDump, sizeof(memoryDump), &size)) {
		for (i=0; i<size; i+=8) {
			int j;
			char digit[3];
			int max;
			max = size - i;
			if (max > 8)
				max = 8;
			tempo[0] = '\0';
			for (j=0; j<max; j++) {
				sprintf(digit, "%.2x ", memoryDump[i + j]);
				strcat(tempo, digit);
			}
			ce_exception_writestring(file, tempo);
		}
	}
#else
	ce_exception_writeBreak(file);
	ce_exception_writestring(file, "Context dump only available on ARM devices");
#endif
}

void ce_exception_dumpException(HANDLE file, EXCEPTION_RECORD *exceptionRecord)
{
	char tempo[200];
	char exceptionName[50];
	unsigned int i;
#if (_WIN32_WCE >= 300)
	ce_exception_writeBreak(file);
	switch(exceptionRecord->ExceptionCode) {
		case EXCEPTION_ACCESS_VIOLATION :
			strcpy(exceptionName, "Access Violation");
			break;
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED :
			strcpy(exceptionName, "Array Bounds Exceeded");
			break;
		case EXCEPTION_DATATYPE_MISALIGNMENT :
			strcpy(exceptionName, "Datatype Misalignment");
			break;
		case EXCEPTION_IN_PAGE_ERROR :
			strcpy(exceptionName, "In Page Error");
			break;
		case EXCEPTION_INT_DIVIDE_BY_ZERO :
			strcpy(exceptionName, "Int Divide By Zero");
			break;
		case EXCEPTION_INT_OVERFLOW :
			strcpy(exceptionName, "Int Overflow");
			break;
		case EXCEPTION_STACK_OVERFLOW :
			strcpy(exceptionName, "Stack Overflow");
			break;
		default:
			sprintf(exceptionName, "%.8lx", exceptionRecord->ExceptionCode);
			break;
	}
	sprintf(tempo, "Exception %s Flags %.8lx Address %.8lx", exceptionName,
		exceptionRecord->ExceptionFlags,
		(long unsigned int)exceptionRecord->ExceptionAddress);
	ce_exception_writestring(file, tempo);
	if (exceptionRecord->NumberParameters) {
		for (i=0; i<exceptionRecord->NumberParameters; i++) {
			sprintf(tempo, "Parameter %d %.8lx", i, exceptionRecord->ExceptionInformation[i]);
			ce_exception_writestring(file, tempo);
		}
	}
	if (exceptionRecord->ExceptionRecord)
		ce_exception_dumpException(file, exceptionRecord->ExceptionRecord);
#else
	ce_exception_writeBreak(file);
	ce_exception_writestring(file, "Cannot get exception information on this CE version");
#endif
}

BOOL ce_exception_write(TCHAR *path, EXCEPTION_POINTERS *exceptionPointers)
{
	HANDLE dumpFile;
	TCHAR dumpFileName[MAX_PATH];
	SYSTEMTIME systemTime;

	GetSystemTime(&systemTime);
	wsprintf(dumpFileName, TEXT("%s_%.2d_%.2d_%.4d_%.2d_%.2d_%.2d.txt"),
			path, systemTime.wDay, systemTime.wMonth, systemTime.wYear,
			systemTime.wHour, systemTime.wMinute, systemTime.wSecond);
	dumpFile = CreateFile(dumpFileName, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (dumpFile == INVALID_HANDLE_VALUE)
		return FALSE;

	ce_exception_dumpException(dumpFile, exceptionPointers->ExceptionRecord);
	ce_exception_dumpContext(dumpFile, GetCurrentProcess(), exceptionPointers->ContextRecord);

	CloseHandle(dumpFile);

	return TRUE;
}
