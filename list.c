/* 
 * File:    list.c
 * 
 * Author:  fossette
 * 
 * Date:    2019/01/15
 *
 * Version: 1.0
 * 
 * Descr:   Package list object. Tested under FreeBSD 11.2.
 *
 *          The package list file, '.pkgcache' by default, is a simple
 *          text file where the first line contains the URL of a FreeBSD
 *          package repository.  The next lines contain the package's
 *          base name, i.e. without the version number.
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
#include <ctype.h>
#include <malloc_np.h>
#include <string.h>

#include "common.h"


/*
 *  Constants
 */

#define  LNPKGNAME                  50
#define  PKGNAMELIST_INITIAL_COUNT  50


/*
 *  Types
 */

typedef char PKGNAME[LNPKGNAME];


/*
 *  Object variables
 */

int      giGet = -1,
         giPkgnameListCount = 0,
         giPkgnameListNextAdd = 0,
         giStatExisting = 0,
         giStatNew = 0;
char     gszPkgRepoUrl[LNFILENAME] = "";
PKGNAME  *gpPkgnameList = NULL;


/*
 *  ListFindInternal
 *
 *  Return: the strcmp() result.
 */

int
ListFindInternal(char *szPkgName, int iMin, int iMax,     int *pIndex)
{
   int   iIndex,
         iCmp;   
   

   if (iMax - iMin < 8)
   {
      // Linear search is optimal in this case
      iIndex = iMin - 1;
      do
      {
         iIndex++;
         iCmp = strcmp(gpPkgnameList[iIndex], szPkgName);
      }
      while (iCmp < 0 && iIndex < iMax) ;

      *pIndex = iIndex;
   }
   else
   {
      // Binary search is optimal in this case
      iIndex = (iMin + iMax) / 2;
      iCmp = strcmp(gpPkgnameList[iIndex], szPkgName);
      if (iCmp)
      {
         if (iCmp < 0)
            iCmp = ListFindInternal(szPkgName, iIndex + 1, iMax,     pIndex);
         else
            iCmp = ListFindInternal(szPkgName, iMin, iIndex - 1,     pIndex);
      }
      else
         *pIndex = iIndex;
   }

   return(iCmp);
}


/*
 *  ListPkgNameValidateInternal
 */

void
ListPkgNameValidateInternal(char *szPkgNameRaw,     char *szPkgName)
{
   int i;
   
   
   if (szPkgNameRaw)
   {
      for (i = LNPKGNAME-1 ; *szPkgNameRaw
                             && (*szPkgNameRaw != '-'
                                 || !isdigit(*(szPkgNameRaw+1)) )
                             && !isspace(*szPkgNameRaw) && i ; i--)
      {
         *szPkgName = *szPkgNameRaw;
         szPkgName++;
         szPkgNameRaw++;
      }
   }

   *szPkgName = 0;
}


/*
 *  ListAdd
 *
 *  Return: ERROR_PKGCACHE_xyz
 */

int
ListAdd(char *szPkgNameRaw)
{
   int      i,
            iCmp,
            iErr = 0,
            iIndex;
   PKGNAME  szPkgName;


   // Clean up the package name
   ListPkgNameValidateInternal(szPkgNameRaw,     szPkgName);
   
   if (*szPkgName)
   {
      // Enough space?
      if (giPkgnameListCount)
      {
         if (giPkgnameListCount - giPkgnameListNextAdd < 2)
         {
            giPkgnameListCount *= 2;
            gpPkgnameList = realloc(gpPkgnameList, sizeof(PKGNAME) * giPkgnameListCount);
            if (!gpPkgnameList)
            {
               giPkgnameListCount = 0;
               iErr = ERROR_PKGCACHE_MEM;
            }
         }
      }
      else
      {
         gpPkgnameList = malloc(sizeof(PKGNAME) * PKGNAMELIST_INITIAL_COUNT);
         if (gpPkgnameList)
            giPkgnameListCount = PKGNAMELIST_INITIAL_COUNT;
         else
            iErr = ERROR_PKGCACHE_MEM;
      }

      if (!iErr)
      {
         // Find out if the package name has already been added
         if (giPkgnameListNextAdd)
         {
            // Optimization check since the list is expected to be sorted.
            iCmp = ListFindInternal(szPkgName, giPkgnameListNextAdd-1,
                                               giPkgnameListNextAdd-1,
                                                  &iIndex);      
            if (iCmp > 0)
               iCmp = ListFindInternal(szPkgName, 0, giPkgnameListNextAdd-1,
                                          &iIndex);
            if (iCmp < 0)
               iIndex++;
         }
         else
         {
            // First package name to add to the list
            iIndex = 0;
            iCmp = 1;
         }

         if (iCmp)
         {
            giGet = -1;    // Invalidate ListGetNext()

            // Shift the entries if needed
            for (i = giPkgnameListNextAdd ; i > iIndex ; i--)
               strcpy(gpPkgnameList[i], gpPkgnameList[i - 1]);

            // Add the package name
            strcpy(gpPkgnameList[iIndex], szPkgName);
            giPkgnameListNextAdd++;
         }
      }

      // Refresh stats
      if (!iErr)
      {
         if (iCmp)
            giStatNew++;
         else
            giStatExisting++;
      }
   }
   // else
   //    exit silently if no package to add

   return(iErr);
}


/*
 *  ListGetNext
 *
 *  Return: TRUE if found.
 */

int
ListGetNext(     char *szPkgName)
{
   int iBool;
   

   if (giGet < 0)
      iBool = 0;
   else
   {
      strcpy(szPkgName, gpPkgnameList[giGet]);
      giGet++;
      iBool = (giGet < giPkgnameListNextAdd);
      if (!iBool)
         giGet = -1;
   }
   
   return(iBool);
}


/*
 *  ListGetFirst
 *
 *  Return: TRUE if found.
 */

int
ListGetFirst(     char *szPkgName)
{
   int iBool;
   

   if (giPkgnameListNextAdd)
   {
      giGet = 0;
      iBool = ListGetNext(     szPkgName);
   }
   else
      iBool = 0;
   
   return(iBool);
}


/*
 *  ListGetRepoUrl
 */


void
ListGetRepoUrl(     char *pRepoUrl)
{
   strcpy(pRepoUrl, gszPkgRepoUrl);
}


/*
 *  ListGetStatExisting
 *
 *  Return: a positive count.
 */


int
ListGetStatExisting(void)
{
   return(giStatExisting);
}


/*
 *  ListGetStatNew
 *
 *  Return: a positive count.
 */


int
ListGetStatNew(void)
{
   return(giStatNew);
}


/*
 *  ListIsFound
 *
 *  Return: TRUE if found.
 */

int
ListIsFound(char *szPkgNameRaw)
{
   int      iIndex,
            iBool;
   PKGNAME  szPkgName;


   // Clean up the package name
   ListPkgNameValidateInternal(szPkgNameRaw,     szPkgName);
   
   if (giPkgnameListNextAdd && *szPkgName)
      iBool = !ListFindInternal(szPkgName, 0, giPkgnameListNextAdd - 1,     &iIndex);
   else
      iBool = 0;
   
#ifdef PKGCACHE_VERBOSE
printf("ListIsFound(%s) iBool=%d\n", szPkgNameRaw, iBool);
#endif

   return(iBool);
}


/*
 *  ListLoad
 *
 *  Return: ERROR_PKGCACHE_xyz
 */

int
ListLoad(char *szFilename)
{
   int      i,
            iErr = 0;
   char     sz[LNSZ],
            szUrl[LNFILENAME],
            *p;
   FILE     *pFile;

   
   pFile = fopen(szFilename, "r");
   if (pFile)
   {
      // Get the package repository URL
      p = fgets(szUrl, LNFILENAME, pFile);
      if (p)
      {
         szUrl[LNFILENAME-2] = 0;
         i = strlen(szUrl);
         if (i)
         {
            if (szUrl[i-1] == '\n')
            {
               i--;
               szUrl[i] = 0;
            }
            if (szUrl[i-1] != '/')
            {
               szUrl[i] = '/';
               i++;
               szUrl[i] = 0;
            }

            strcpy(gszPkgRepoUrl, szUrl);
         }
      }

      // Get the package name list
      while (!iErr && p)
      {
         p = fgets(sz, LNPKGNAME, pFile);
         if (p)
         {
            sz[LNPKGNAME-1] = 0;            
            iErr = ListAdd(sz);
         }
      }

      // Done!
      fclose(pFile);

      // Reset stats because the LOAD phase does not count.
      giStatExisting = 0;
      giStatNew = 0;

   }

   return(iErr);
}


/*
 *  ListQuit
 */

void
ListQuit(void)
{
   if (gpPkgnameList)
   {
      free(gpPkgnameList);
      gpPkgnameList = NULL;
   }
}


/*
 *  ListSave
 *
 *  Return: ERROR_PKGCACHE_xyz
 */

int
ListSave(char *szFilename)
{
   int      i,
            iEof,
            iErr = 0;
   FILE     *pFile;
   PKGNAME  *p;

   
   pFile = fopen(szFilename, "w");
   if (pFile)
   {
      iEof = fputs(gszPkgRepoUrl, pFile);
      if (iEof >= 0)
         iEof = fputs("\n", pFile);

      p = gpPkgnameList;
      for (i = 0 ; i < giPkgnameListNextAdd && iEof >= 0 ; i++)
      {
         iEof = fputs(*p, pFile);
         p++;
         if (iEof >= 0)
            iEof = fputs("\n", pFile);
      }
      
      if (iEof < 0)
         iErr = ERROR_PKGCACHE_FILE_W;

      // Done!
      fclose(pFile);
   }
   else
      iErr = ERROR_PKGCACHE_ACCESS;

   return(iErr);
}

