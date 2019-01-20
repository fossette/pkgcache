/* 
 * File:    pkgcache.c
 * 
 * Author:  fossette
 * 
 * Date:    2019/01/19
 *
 * Version: 1.0
 * 
 * Descr:   FreeBSD 'pkg' cache implementation to facilitate easy offline
 *          packages installation. Tested under FreeBSD 11.2.
 * 
 *          First parameter: the tool's command
 *            Add : Interactively add packages to the package list.
 *            Create : Create/update the package list using 'pkg info'.
 *                If the directory and filename path is not specified,
 *                '.pkgcache' located in the current directory is used.
 *            Download : Update the packages cache via Internet.  The
 *                package list must have previously been created, and
 *                its first line must contain the URL of a FreeBSD
 *                package repository.  Package dependancies are
 *                automatically added to the package list.
 *            Help : Display the tool's command syntax.
 * 
 *          Optional parameter: the packages directory and package list
 *                filename path to use.  If the packages directory is
 *                not specified, the current directory is used.  If the
 *                package list filename is not specified, '.pkgcache'
 *                is used.
 *
 *          The package list file, '.pkgcache' by default, is a simple
 *          text file where the first line contains the URL of a FreeBSD
 *          package repository.  The next lines contain the package's
 *          base name, i.e. without the version number.
 *
 *          This program uses the specified or current directory to
 *          hold temporary files.
 *
 * Web:     https://github.com/fossette/pkgcache/wiki
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <archive.h>
#include <archive_entry.h>
#include <ctype.h>
#include <errno.h>
#include <archive.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/stat.h>

#include <fetch.h>

#include "common.h"
#include "list.h"


/*
 *  Constants
 */

#define ENVHTTPTIMEOUT              "HTTP_TIMEOUT"
#define LNBLOCK                     2048

#define PKGCACHE_DEFAULT_FILENAME   ".pkgcachelist"
#define PKGCACHE_TEMP_FILENAME      ".pkgcachetemp"

#define PKGCACHE_ADD                1
#define PKGCACHE_CREATE             2
#define PKGCACHE_DOWNLOAD           3
#define PKGCACHE_HELP               4

#define PKGCACHE_DEPS_DEFAULT       0b000000
#define PKGCACHE_DEPS_QUOTE         0b000001

#define PKGCACHE_HTML_DEFAULT       0b0000000
#define PKGCACHE_HTML_QUOTE         0b0000001
#define PKGCACHE_HTML_TAG           0b0000010
#define PKGCACHE_HTML_TAGQUOTE      0b0000011
#define PKGCACHE_HTML_HREF          0b0000111
#define PKGCACHE_HTML_ATAG          0b0001010
#define PKGCACHE_HTML_ATAGQUOTE     0b0001011
#define PKGCACHE_HTML_BEGINTAG      0b0010010
#define PKGCACHE_HTML_CLOSETAG      0b0100010
#define PKGCACHE_HTML_EOH           0b1000000

#define LNPKGCACHE_DOWNLOAD_ALWAYS  6
char *PKGCACHE_DOWNLOAD_ALWAYS[LNPKGCACHE_DOWNLOAD_ALWAYS]
   = {"digests.txz", "meta.txz", "packagesite.txz", "pkg-devel.txz", "pkg.txz", "pkg.txz.sig"};


/*
 *  GetDepsEntry
 *
 *  Return: TRUE if found
 */

int
GetDepsString(const la_ssize_t iCountR, char *pBlock,
                  int *piBlock, int *piState, int *pLevel, char *pSz)
{
   int   iBool = 0,
         iOut;
   char  *pIn,
         *pOut;


   pIn = pBlock + *piBlock;
   iOut = strlen(pSz);
   pOut = pSz + iOut;
   while (*piBlock < iCountR && !iBool)
   {
      if (*piState == PKGCACHE_DEPS_QUOTE)
      {
         if (*pIn == '"')     // This is the end-quote
         {
            iBool = 1;
            *piState = PKGCACHE_DEPS_DEFAULT;
         }
         else
         {
            *pOut = *pIn;
            iOut++;
            pOut++;
            if (iOut >= LNSZ-1)
               iBool = 1;     // Too long!  Trunk and return it!
         }
      }
      else if (*pIn == '"')
         *piState = PKGCACHE_DEPS_QUOTE;
      else if (*pIn == '{')
         (*pLevel)++;
      else if (*pIn == '}')
         (*pLevel)--;

      pIn++;
      (*piBlock)++;
   }

   if (*piBlock >= iCountR)
      *piBlock = 0;

   *pOut = 0;

   return(iBool);
}


/*
 *  CheckDependancies
 *
 *  Return: ERROR_PKGCACHE_xyz
 */

int
CheckDependancies(const char *pFilename)
{
   int         iBlock,
               iErr = 0,
               iDeps,
               iDepsCount,
               iDepsLevel,
               iLevel,
               iState;
   la_ssize_t  iCountR;
   char        block[LNBLOCK],
               szDeps[LNSZ];
   struct archive *pArc;
   struct archive_entry *pArcEntry;


   pArc = archive_read_new();
   pArcEntry = archive_entry_new();
   if (pArc && pArcEntry)
   {
      iErr = archive_read_support_filter_all(pArc);
      if (!iErr)
         iErr = archive_read_support_format_all(pArc);
      if (!iErr)
         iErr = archive_read_open_filename(pArc, pFilename, LNBLOCK);
      do
      {
         iErr = archive_read_next_header2(pArc,     pArcEntry);
         if (iErr == ARCHIVE_WARN)
            iErr = 0;
         if (!iErr)
         {
#ifdef PKGCACHE_VERBOSE
printf("CheckDependancies(%s): archive_entry_pathname=%s\n", pFilename,
        archive_entry_pathname(pArcEntry));
#endif
            if (strcmp(archive_entry_pathname(pArcEntry), "+MANIFEST"))
               archive_read_data_skip(pArc);
            else
            {
               iBlock = 0;
               iDeps = 0;
               iLevel = 0;
               iState = PKGCACHE_DEPS_DEFAULT;
               *szDeps = 0;
               do
               {
                  iCountR = archive_read_data(pArc, block, LNBLOCK);
                  if (iCountR < 0)
                  {
                     iErr = iCountR;
                     iCountR = 0;
                  }
                  if (iCountR)
                  {
                     do
                     {
                        if (GetDepsString(iCountR, block,
                                 &iBlock, &iState, &iLevel, szDeps))
                        {
#ifdef PKGCACHE_VERBOSE
printf("CheckDependancies(%s): GetDepsString=%s\n", pFilename, szDeps);
#endif
                           if (iDeps)
                           {
                              if (iLevel == iDepsLevel && !iDepsCount)
                              {
                                 iErr = ListAdd(szDeps);
                                 if (!iErr)
                                    iErr = ARCHIVE_EOF;
                              }
                              else if (iLevel == iDepsLevel + 1)
                              {
                                 iErr = ListAdd(szDeps);
                                 if (!iErr)
                                    iDepsCount++;
                              }
                              else if (iLevel <= iDepsLevel)
                                 iErr = ARCHIVE_EOF;
                           }
                           else if (!strcmp(szDeps, "deps"))
                           {
                              iDeps++;
                              iDepsCount = 0;
                              iDepsLevel = iLevel;
                           }

                           *szDeps = 0;
                        }
                     }
                     while (iBlock && !iErr) ;
                  }
               }
               while (iCountR == LNBLOCK && !iErr) ;

               if (!iErr)
                  iErr = ARCHIVE_EOF;
            }
         }
      }
      while (!iErr) ;
      if (iErr == ARCHIVE_EOF)
         iErr = 0;
   }

   if (pArcEntry)      
      archive_entry_free(pArcEntry);
   if (pArc)      
      archive_read_free(pArc);

   return(iErr);
}


/*
 *  CompareCommand
 *
 *  Return: TRUE if identical.
 */

int
CompareCommand(const char *szCmd, const char *sz)
{
   int   iCmd,
         iRet,
         iSz;

   
   iCmd = strlen(szCmd);
   iSz = strlen(sz);
   iRet = (iCmd && iSz && iSz <= iCmd);
   if (iRet)
      do
      {
         iSz--;
         iRet = (szCmd[iSz] == toupper(sz[iSz]));
      }
      while (iRet && iSz);

   return(iRet);
}


/*
 *  DownloadFile
 *
 *  Return: ERROR_PKGCACHE_xyz
 */

int
DownloadFile(const char *pUrl, const char *pFilename)
{
   int      iErr = 0;
   char     block[LNBLOCK];
   size_t   iCountR,
            iCountW;
   FILE     *pFileR,
            *pFileW;


   if (!Exist(pFilename, PKGCACHE_EXIST_FILE))
   {
      pFileW = fopen(pFilename, "w");
      if (pFileW)
      {
#ifdef PKGCACHE_VERBOSE
printf("DownloadFile: fopen(%s) OK\n", pFilename);
#endif
         pFileR = fetchGetURL(pUrl, "");
      }
      else
         pFileR = NULL;

      if (pFileR)
      {
#ifdef PKGCACHE_VERBOSE
printf("DownloadFile: fetchGetURL(%s) OK\n", pUrl);
#endif
         do
         {
            iCountR = fread(block, 1, LNBLOCK, pFileR);
            if (iCountR)
            {
               iCountW = fwrite(block, 1, iCountR, pFileW);
               if (iCountR != iCountW)
                  iErr = ERROR_PKGCACHE_FILE_W;
            }
         }
         while (iCountR == LNBLOCK && !iErr) ;

         // Check for a successful download
         if (iCountR < LNBLOCK && !iErr)
         {
            if (!feof(pFileR) || ferror(pFileR))
               iErr = ERROR_PKGCACHE_FILE_R;
         }
      }

      if (pFileR)
         fclose(pFileR);
      if (pFileW)
         fclose(pFileW);
      else
      {
#ifdef PKGCACHE_VERBOSE
printf("DownloadFile: fopen(%s) errno=%d\n", pFilename, errno);
#endif
      }
      if (iErr)
      {
         // Cleanup if incomplete download, but make sure it's a file.
         if (Exist(pFilename, PKGCACHE_EXIST_FILE))
            remove(pFilename);
      }
   }

#ifdef PKGCACHE_VERBOSE
if (iErr)
   printf("DownloadFile: iErr=%d\n", iErr);
#endif

   return(iErr);
}


/*
 *  GetHrefLink
 *
 *  Return: TRUE if link found
 */

int
GetHrefLink(const int iCountR, char *pBlock,
                  int *piBlock, int *piState, char *pszHref)
{
   int   iBool = 0,
         iOut;
   char  *pIn,
         *pOut;


   pIn = pBlock + *piBlock;
   iOut = strlen(pszHref);
   pOut = pszHref + iOut;
   while (*piBlock < iCountR && !iBool && *piState != PKGCACHE_HTML_EOH)
   {
      // Handeling quoted text
      if (*piState & PKGCACHE_HTML_QUOTE)
      {
         if (*pIn == '"')  // This is the end-quote
         {
            if (*piState == PKGCACHE_HTML_HREF)
            {
               iBool = 1;
               *piState = PKGCACHE_HTML_TAG;
            }
            else
               *piState &= PKGCACHE_HTML_ATAG; // & with implicit PKGCACHE_HTML_TAG
         }
         else
         {
            if (*piState == PKGCACHE_HTML_HREF)
            {
               if (iOut >= LNSZ-1)
               {
                  // Too long, skip it!
                  *piState = PKGCACHE_HTML_TAGQUOTE;
               }
               else
               {
                  *pOut = *pIn;
                  iOut++;
                  pOut++;
               }
            }
         }
      }

      // Handeling tags
      else if (*piState & PKGCACHE_HTML_TAG)
      {
         if (*pIn == '>')  // This is the end-tag
         {
            if (*piState == PKGCACHE_HTML_CLOSETAG)
            {
               *pOut = 0;
               if (strcasecmp(pszHref, "/html"))
                  *piState = PKGCACHE_HTML_DEFAULT;
               else
                  *piState = PKGCACHE_HTML_EOH;
            }
            else
               *piState = PKGCACHE_HTML_DEFAULT;
         }
         else if (*piState == PKGCACHE_HTML_BEGINTAG)
         {
            if (isspace(*pIn))
            {
               *pOut = 0;
               if (strcasecmp(pszHref, "a"))
                  *piState = PKGCACHE_HTML_TAG;
               else
               {
                  pOut = pszHref;
                  iOut = 0;
                  *piState = PKGCACHE_HTML_ATAG;
               }
            }
            else if (*pIn == '"')
            {
               *piState = PKGCACHE_HTML_TAGQUOTE;
            }
            else if (*pIn == '/')
            {
               pOut = pszHref;
               *pOut = '/';
               pOut++;
               iOut = 1;
               *piState = PKGCACHE_HTML_CLOSETAG;
            }
            else
            {
               *pOut = *pIn;
               iOut++;
               pOut++;
            }
         }
         else if (*piState == PKGCACHE_HTML_ATAG)
         {
            if (isspace(*pIn))
            {
               // Tolerate a space before and after the '=' sign only.
               *pOut = 0;
               if (strcasecmp(pszHref, "href") && strcasecmp(pszHref, "href="))
               {
                  pOut = pszHref;
                  iOut = 0;
               }
            }
            else if (*pIn == '"')
            {
               *pOut = 0;
               if (strcasecmp(pszHref, "href="))
                  *piState = PKGCACHE_HTML_ATAGQUOTE;
               else
                  *piState = PKGCACHE_HTML_HREF;

               pOut = pszHref;
               iOut = 0;
            }
            else
            {
               *pOut = *pIn;
               iOut++;
               pOut++;
            }
         }
         else if (*piState == PKGCACHE_HTML_CLOSETAG)
         {
            *pOut = *pIn;
            iOut++;
            pOut++;
         }
      }

      // Waiting for tags
      else
      {
         if (*pIn == '<')  // This is the begin-tag
         {
            pOut = pszHref;
            iOut = 0;
            *piState = PKGCACHE_HTML_BEGINTAG;
         }
      }

      pIn++;
      (*piBlock)++;
   }

   if (*piBlock >= iCountR)
      *piBlock = 0;

   *pOut = 0;

   return(iBool);
}


/*
 *  IsDownloadAlways
 *
 *  Return: TRUE if identified.
 */

int
IsDownloadAlways(const char *szFilename)
{
   int   i =      LNPKGCACHE_DOWNLOAD_ALWAYS,
         iBool =  0;


   do
   {
      i--;
      iBool = !strcasecmp(szFilename, PKGCACHE_DOWNLOAD_ALWAYS[i]);
   }
   while (i && !iBool) ;

   return(iBool);
}


/*
 *  DownloadUpdates
 *
 *  Return: ERROR_PKGCACHE_xyz
 */

int
DownloadUpdates(const char *pUrl, const char *pPkgcachePathname)
{
   int      i,
            iBlock = 0,
            iErr = 0,
            iHrefLn,
            iState = PKGCACHE_HTML_DEFAULT;
   char     block[LNBLOCK];
   size_t   iCountR,
            iCountW;
   FILE     *pFileR,
            *pFileW = NULL;
   FILENAME szHref,
            szPkgcachePathname2,
            szTempname,
            szUrl2;


   // Since the docs say that fetch is not reentrant,
   // no choice but to read the web page twice.
   iErr = MakePath(pPkgcachePathname);
   if (!iErr)
   {
      strcpy(szTempname, pPkgcachePathname);
      i = strlen(szTempname);
      if (szTempname[i - 1] != '/')
      {
         szTempname[i] = '/';
         i++;
         szTempname[i] = 0;
      }
      strcpy(szTempname + i, PKGCACHE_TEMP_FILENAME);

      pFileW = fopen(szTempname, "w");
      if (!pFileW)
         iErr = ERROR_PKGCACHE_ACCESS;
   }
   if (!iErr)
   {
      pFileR = fetchGetURL(pUrl, "");
      if (!pFileR)
         iErr = ERROR_PKGCACHE_REPO;
   }
   if (!iErr)
   {
#ifdef PKGCACHE_VERBOSE
printf("DownloadUpdates: fetchGetURL(%s) OK\n", pUrl);
#endif
      do
      {
         iCountR = fread(block, 1, LNBLOCK, pFileR);
         if (iCountR)
         {
            iCountW = fwrite(block, 1, iCountR, pFileW);
            if (iCountR != iCountW)
               iErr = ERROR_PKGCACHE_FILE_W;
         }
      }
      while (iCountR == LNBLOCK && !iErr) ;

      fclose(pFileR);
   }
   if (pFileW)
      fclose(pFileW);
   
   // Second read...
   if (!iErr)
   {
      pFileR = fopen(szTempname, "r");
      if (!pFileR)
         iErr = ERROR_PKGCACHE_ACCESS;
   }
   if (!iErr)
   {
      szHref[0] = 0;
      do
      {
         iCountR = fread(block, 1, LNBLOCK, pFileR);
         do
         {
            if (GetHrefLink(iCountR, block,     &iBlock, &iState, szHref))
            {
               iHrefLn = strlen(szHref);
               if (iHrefLn)
               {
                  if (szHref[0] != '/' && strncasecmp(szHref, "http:", 5)
                      && strncasecmp(szHref, "https:", 6)
                      && szHref[0] != '.')  // Skip absolute and navigation links
                  {
                     sprintf(szUrl2, "%s%s", pUrl, szHref);
                     sprintf(szPkgcachePathname2, "%s%s", pPkgcachePathname,
                             szHref);
                     if (szHref[iHrefLn-1] == '/')
                        iErr = DownloadUpdates(szUrl2, szPkgcachePathname2);
                     else
                     {
                        if (IsDownloadAlways(szHref)
                            || (ListIsFound(szHref)
                                && !Exist(szPkgcachePathname2, PKGCACHE_EXIST_FILE)) )
                        {
                           printf("Downloading %s\n", szHref);
                           iErr = MakePath(pPkgcachePathname);
                           if (!iErr)
                              iErr = DownloadFile(szUrl2, szPkgcachePathname2);
                           if (!iErr)
                              iErr = CheckDependancies(szPkgcachePathname2);
                           else if (iErr == ERROR_PKGCACHE_FILE_R)
                           {
                              printf("WARNING: Skipping %s, download failed!\n",
                                     szHref);
                              iErr = 0;
                           }
                        }
                     }
                  }

                  szHref[0] = 0;
               }
            }
         }
         while (iBlock && !iErr && iState != PKGCACHE_HTML_EOH) ;
      }
      while (iCountR == LNBLOCK && !iErr && iState != PKGCACHE_HTML_EOH) ;

      fclose(pFileR);

      if (!iErr && iState != PKGCACHE_HTML_EOH)
      {
         printf("ERROR: %s didn't load completely.\n", pUrl);
         iErr = ERROR_PKGCACHE_NO_EOH;
      }
   }

   remove(szTempname);
   return(iErr);
}


/*
 *  Main
 */

int
main(int argc, char** argv)
{
   int            i,
                  iCommand = 0,
                  iErr = 0,
                  iNew;
   char           sz[LNSZ],
                  *p;
   FILE           *pFile;
   FILENAME       szPkgcachePathname,
                  szPkglistFilename,
                  szPkgRepoUrl,
                  szResultsFilename,
                  szTempName;
   struct stat    sPathStats;


   // Initialisation
   printf("\npkgcache v1.0\n"
            "-------------\n"
            "  https://github.com/fossette/pkgcache/wiki\n\n");
   
   // Make sure HTTP_TIMEOUT has some decent value.
   p = getenv(ENVHTTPTIMEOUT);
   if (p)
      if (atoi(p) < 2)
         setenv(ENVHTTPTIMEOUT, "180", 1);   // 3 minutes timeout.

   // Generate the default pkg cache pathname
   p = getcwd(szPkgcachePathname, LNFILENAME-20);
   szPkgcachePathname[LNFILENAME-20] = 0;
   i = strlen(szPkgcachePathname);
   if (!i || i == LNFILENAME-20 || !p)
      iErr = ERROR_PKGCACHE_MEM;
   if (!iErr)
   {
      if (szPkgcachePathname[i-1] != '/')
      {
         szPkgcachePathname[i] = '/';
         i++;
         szPkgcachePathname[i] = 0;
      }

      // Generate the default pkg list filename
      strcpy(szPkglistFilename, szPkgcachePathname);
      strcpy(szPkglistFilename + i, PKGCACHE_DEFAULT_FILENAME);

      // Option parsing
      if (argc >= 2 && argc <= 6)
      {
         i = 1;

         // Option: Set HTTP_TIMEOUT
         if (*(argv[i]) == '-' && argc >= 4)
         {
            if (CompareCommand("TIMEOUT", (argv[i])+1)
                && atoi(argv[i+1]) > 1)
            {
               setenv(ENVHTTPTIMEOUT, argv[i+1], 1);
               i++;
            }            
            i++;
         }
         
         // Extract the tool's command
         if (i < argc)
         {
            if (CompareCommand("ADD", argv[i]))
               iCommand = PKGCACHE_ADD;
            else if (CompareCommand("CREATE", argv[i]))
               iCommand = PKGCACHE_CREATE;
            else if (CompareCommand("DOWNLOAD", argv[i]))
               iCommand = PKGCACHE_DOWNLOAD;
            else if (CompareCommand("HELP", argv[i]))
               iCommand = PKGCACHE_HELP;
            else
               iErr = ERROR_PKGCACHE_CMD;

            i++;
         }
         else
            iErr = ERROR_PKGCACHE_CMD;

         // Extract optional path
         if (!iErr && i < argc)
         {
            StrnCopy(szTempName, argv[i], LNFILENAME);

            i = strlen(szTempName);
            if (i == LNFILENAME-1)
               iErr = ERROR_PKGCACHE_MEM;
            else if (i)
            {
               if (stat(szTempName,     &sPathStats))
               {
                  if (iCommand == PKGCACHE_DOWNLOAD)
                     iErr = ERROR_PKGCACHE_ACCESS;
               }
               else
               {
                  if (S_ISDIR(sPathStats.st_mode))
                  {
                     if (i < LNFILENAME-20)
                     {
                        strcpy(szPkgcachePathname, szTempName);
                        if (szPkgcachePathname[i-1] != '/')
                        {
                           szPkgcachePathname[i] = '/';
                           i++;
                           szPkgcachePathname[i] = 0;
                        }

                        strcpy(szPkglistFilename, szPkgcachePathname);
                        strcpy(szPkglistFilename + i, PKGCACHE_DEFAULT_FILENAME);
                     }
                     else
                        iErr = ERROR_PKGCACHE_MEM;
                  }
                  else
                  {
                     strcpy(szPkglistFilename, szTempName);
                     do
                     {
                        i--;
                        if (szTempName[i] != '/')
                           szTempName[i] = 0;
                     }
                     while (i && !(szTempName[i]));

                     if (szTempName[i])
                        strcpy(szPkgcachePathname, szTempName);
                  }
               }
            }
         }
      }
      else
         iErr = ERROR_PKGCACHE_CMD;
   }
   
   if (!iErr)
   {
      sprintf(szResultsFilename, "%s%s",
              szPkgcachePathname, PKGCACHE_TEMP_FILENAME);
      
      printf("Package List: %s\n", szPkglistFilename);
      iErr = ListLoad(szPkglistFilename);
   }
   if (!iErr)
   {
      switch (iCommand)
      {
         case PKGCACHE_ADD:
            printf("\nEnter package names, one per line, an empty line to quit!\n");
            *sz = 0;
            do
            {
               p = gets_s(sz, LNSZ);
               if (p)
               {
                  sz[LNSZ-1] = 0;
                  iErr = ListAdd(sz);
               }
            }
            while (!iErr && p && *sz) ;
            break;

         case PKGCACHE_CREATE:
            sprintf(szTempName, "pkg info > %s", szResultsFilename);
            if (system(szTempName))
               iErr = ERROR_PKGCACHE_INFO;
            else
            {
               pFile = fopen(szResultsFilename, "r");
               if (pFile)
               {
                  do
                  {
                     p = fgets(sz, LNSZ, pFile);
                     if (p)
                     {
                        sz[LNSZ-1] = 0;            
                        iErr = ListAdd(sz);
                     }
                  }
                  while (!iErr && p) ;

                  // Done!
                  fclose(pFile);
               }
            }
            remove(szResultsFilename);
            break;

         case PKGCACHE_DOWNLOAD:
            ListGetRepoUrl(     szPkgRepoUrl);
            if (*szPkgRepoUrl)
            {
               iNew = ListGetStatNew();
               do
               {
                  i = iNew;
                  iErr = DownloadUpdates(szPkgRepoUrl, szPkgcachePathname);
                  if (iErr == ERROR_PKGCACHE_NO_EOH)
                  {
                     p = getenv(ENVHTTPTIMEOUT);
                     if (p)
                     {
                        if (atoi(p) > 1)
                           printf("HTTP Fetch Timeout: %s sec., ", p);
                        else
                           printf("No HTTP Fetch Timeout specified! ");
                     }
                     printf("Retry? ([CR]=Yes) ");
                     gets_s(sz, LNSZ);
                     if (!(*sz) || *sz=='y' || *sz=='Y')
                     {
                        i--;
                        iErr = 0;
                     }
                  }
                  iNew = ListGetStatNew();
               }
               while (iNew != i && !iErr) ;
            }
            else
               iErr = ERROR_PKGCACHE_REPO;
            break;
      }
   }
   if (!iErr)
      iErr = ListSave(szPkglistFilename);
   if (!iErr)
   {
      iNew = ListGetStatNew();
      printf("Stats: %d new package", iNew);
      if (iNew > 1)
         printf("s");
      i = ListGetStatExisting();
      printf(" added, %d existing package", i);
      if (i > 1)
         printf("s");
      printf(" revisited.\n\n");
   }
   
   switch (iErr)
   {         
      case ERROR_PKGCACHE_ACCESS:
         printf("ERROR: The specified path can't be accessed!\n\n");
         break;
         
      case ERROR_PKGCACHE_CMD:
         printf("ERROR: Invalid Command!\n\n");
         break;
         
      case ERROR_PKGCACHE_FILE_R:
         printf("ERROR: File Download Failed!\n\n");
         break;
         
      case ERROR_PKGCACHE_FILE_W:
         printf("ERROR: File Write Failed!\n\n");
         break;
         
      case ERROR_PKGCACHE_INFO:
         printf("ERROR: Can't fetch 'pkg info' results!"
                "  Workaround: Use the ADD command!\n\n");
         break;
         
      case ERROR_PKGCACHE_MEM:
         printf("ERROR: Out of memory!\n\n");
         break;
         
      case ERROR_PKGCACHE_REPO:
         printf("ERROR: The repository URL is missing from the package list!\n\n");
         break;
   }
   if (iErr == ERROR_PKGCACHE_CMD || iCommand == PKGCACHE_HELP)
      printf("USAGE: pkgcache [-timeout <sec.>] <command> [package-list-filename]\n"
             "  where COMMAND is:\n"
             "    add      : Interactively add packages to the package list.\n"
             "    create   : Create the package list using 'pkg info'.\n"
             "    download : Download relevant packages via Internet.\n"
             "    help     : Display this command syntax page.\n"
             "  Note that the first letter of options and commands is accepted.\n\n");
   
   return(iErr ? EXIT_FAILURE : EXIT_SUCCESS);
}


