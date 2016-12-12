/*
 * Copyright (c) 2010-2016 Stephane Poirier
 *
 * stephane.poirier@oifii.org
 *
 * Stephane Poirier
 * 3532 rue Ste-Famille, #3
 * Montreal, QC, H2X 2L1
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#define WIN32_LEAN_AND_MEAN

#include <stdio.h>
#include <stdlib.h>
//#include <fstream.h>
#include <fstream>
//#include <iostream.h>
#include <iostream>
#include <windows.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

// Insist on at least Winsock v1.1
/*
const VERSION_MAJOR = 1;
const VERSION_MINOR = 1;
*/
const int VERSION_MAJOR = 1;
const int VERSION_MINOR = 1;

#define CRLF "\r\n"                 // carriage-return/line feed pair

void ShowUsage(void)
{
  cout << "Usage: SPISENDMAIL mailserv to_addr from_addr messagefile subjectline" << endl
       << "Example: SPISENDMAIL smtp.myisp.com rcvr@elsewhere.com my_id@mydomain.com message.txt \"this is my subject line\"" << endl;

  exit(1);
}

// Basic error checking for send() and recv() functions
void Check(int iStatus, char *szFunction)
{
  if((iStatus != SOCKET_ERROR) && (iStatus))
    return;

  cerr << "Error during call to " << szFunction << ": " << iStatus << " - " << GetLastError() << endl;
}

int main(int argc, char *argv[])
{
  int         iProtocolPort        = 0;
  char        szSmtpServerName[255] = "";
  char        szToAddr[255]         = "";
  char        szFromAddr[255]       = "";
  char        szBuffer[4096]        = "";
  char        szLine[255]           = "";
  char        szMsgLine[255]        = "";
  char        szSubject[255]        = "";
  SOCKET      hServer;
  WSADATA     WSData;
  LPHOSTENT   lpHostEntry;
  LPSERVENT   lpServEntry;
  SOCKADDR_IN SockAddr;

  // Check for four command-line args
  //if(argc != 5)
  if(argc != 6)
    ShowUsage();

  //cout<<"smtp: "<<argv[1]<<endl<<"to: "<<argv[2]<<endl<<"from: "<<argv[3]<<endl<<"msg file: "<<argv[4];
  cout<<"smtp: "<<argv[1]<<endl<<"to: "<<argv[2]<<endl<<"from: "<<argv[3]<<endl<<"msg file: "<<argv[4]<<endl<<"subject: "<<argv[5];

  // Load command-line args
  lstrcpy(szSmtpServerName, argv[1]);
  lstrcpy(szToAddr, argv[2]);
  lstrcpy(szFromAddr, argv[3]);
  lstrcpy(szSubject, argv[5]);

  // Create input stream for reading email message file
  ifstream MsgFile(argv[4]);
  ifstream ToAddressFile("toaddress.txt");



  ////////////////////////////////////////////////////////////////////////////////////////////
  // get each destination email address (using supplied text file, one email address per line)
  ToAddressFile.getline(szToAddr, sizeof(szToAddr)); //get first email address

  do         // for each line of message text...
  {
	// Attempt to intialize WinSock (1.1 or later)
	if(WSAStartup(MAKEWORD(VERSION_MAJOR, VERSION_MINOR), &WSData))
	{
	cout << "Cannot find Winsock v" << VERSION_MAJOR << "." << VERSION_MINOR << " or later!" << endl;

	return 1;
	}

	// Lookup email server's IP address.
	lpHostEntry = gethostbyname(szSmtpServerName);
	if(!lpHostEntry)
	{
	cout << "Cannot find SMTP mail server " << szSmtpServerName << endl;

	return 1;
	}

	///////////////////////////////////////////////
	// Create a TCP/IP socket, no specific protocol
	hServer = socket(PF_INET, SOCK_STREAM, 0);
	if(hServer == INVALID_SOCKET)
	{
	cout << "Cannot open mail server socket" << endl;

	return 1;
	}

	// Get the mail service port
	lpServEntry = getservbyname("mail", 0);

	// Use the SMTP default port if no other port is specified
	if(!lpServEntry)
	iProtocolPort = htons(IPPORT_SMTP);
	else
	iProtocolPort = lpServEntry->s_port;

	// Setup a Socket Address structure
	SockAddr.sin_family = AF_INET;
	SockAddr.sin_port   = iProtocolPort;
	SockAddr.sin_addr   = *((LPIN_ADDR)*lpHostEntry->h_addr_list);

	// Connect the Socket
	if(connect(hServer, (PSOCKADDR) &SockAddr, sizeof(SockAddr)))
	{
	cout << "Error connecting to Server socket" << endl;

	return 1;
	}

	// Receive initial response from SMTP server
	Check(recv(hServer, szBuffer, sizeof(szBuffer), 0), "recv() Reply");

	// Send HELO server.com
	sprintf(szMsgLine, "HELO %s%s", szSmtpServerName, CRLF);
	Check(send(hServer, szMsgLine, strlen(szMsgLine), 0), "send() HELO");
	Check(recv(hServer, szBuffer, sizeof(szBuffer), 0), "recv() HELO");

	// Send MAIL FROM: <sender@mydomain.com>
	sprintf(szMsgLine, "MAIL FROM:<%s>%s", szFromAddr, CRLF);
	Check(send(hServer, szMsgLine, strlen(szMsgLine), 0), "send() MAIL FROM");
	Check(recv(hServer, szBuffer, sizeof(szBuffer), 0), "recv() MAIL FROM");

	// Send RCPT TO: <receiver@domain.com>
	sprintf(szMsgLine, "RCPT TO:<%s>%s", szToAddr, CRLF);
	Check(send(hServer, szMsgLine, strlen(szMsgLine), 0), "send() RCPT TO");
	Check(recv(hServer, szBuffer, sizeof(szBuffer), 0), "recv() RCPT TO");

	// Send DATA
	sprintf(szMsgLine, "DATA%s", CRLF);
	Check(send(hServer, szMsgLine, strlen(szMsgLine), 0), "send() DATA");
	Check(recv(hServer, szBuffer, sizeof(szBuffer), 0), "recv() DATA");
	//strat writing about the subject, end it with two CRLF chars and after that you can 
	//write data to the body oif the message
	//sprintf(szMsgLine, "Subject: My own subject %s%s", CRLF, CRLF);
	sprintf(szMsgLine, "Subject: %s%s%s", szSubject, CRLF, CRLF);
	Check(send(hServer, szMsgLine, strlen(szMsgLine), 0), "send() DATA");

	// Send all lines of message body (using supplied text file)
	MsgFile.getline(szLine, sizeof(szLine));             // Get first line

	do         // for each line of message text...
	{
		sprintf(szMsgLine, "%s%s", szLine, CRLF);
		Check(send(hServer, szMsgLine, strlen(szMsgLine), 0), "send() message-line");
		MsgFile.getline(szLine, sizeof(szLine)); // get next line.
	} while(MsgFile.good());
	MsgFile.clear();
	MsgFile.seekg(0);


	// Send blank line and a period
	sprintf(szMsgLine, "%s.%s", CRLF, CRLF);
	Check(send(hServer, szMsgLine, strlen(szMsgLine), 0), "send() end-message");
	Check(recv(hServer, szBuffer, sizeof(szBuffer), 0), "recv() end-message");

	// Send QUIT
	sprintf(szMsgLine, "QUIT%s", CRLF);
	Check(send(hServer, szMsgLine, strlen(szMsgLine), 0), "send() QUIT");
	Check(recv(hServer, szBuffer, sizeof(szBuffer), 0), "recv() QUIT");

	// Report message has been sent
	cout << endl << "Sent " << argv[4] << " as email message to " << szToAddr << endl;

	// Close server socket and prepare to exit.
	closesocket(hServer);

	WSACleanup();

	Sleep(10000);

	ToAddressFile.getline(szToAddr, sizeof(szToAddr)); //get next email address
  } while(ToAddressFile.good());





  return 0;
}