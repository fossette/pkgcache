/* 
 * File:    common.c
 * 
 * Author:  fossette
 * 
 * Date:    2019/01/15
 *
 * Version: 1.0
 * 
 * Descr:   Common functions. Tested under FreeBSD 11.2.
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
#include <string.h>
#include <sys/stat.h>

#include "common.h"


/*
 *  Exist
 *
 *  Return: TRUE if found.
 */

int
Exist(const char *szPathname, const int iPathType)
{
   int         iBool;
   struct stat sPathStats;
  
  
   iBool = !stat(szPathname, &sPathStats);
   if (iBool)
   {
      if (iPathType == PKGCACHE_EXIST_FILE)
         iBool = S_ISREG(sPathStats.st_mode);
      else if (iPathType == PKGCACHE_EXIST_DIR)
         iBool = S_ISDIR(sPathStats.st_mode);
   }

#ifdef PKGCACHE_VERBOSE
printf("Exist(%s, %d) iBool=%d\n", szPathname, iPathType, iBool);
#endif

   return(iBool);
}


/*
 *  MakePath
 *
 *  Return: ERROR_PKGCACHE_xyz
 */

int
MakePath(const char *szPathname)
{
   int      i,
            iErr = 0;
   char     *p;
   FILENAME szSubPath;


   if (!Exist(szPathname, PKGCACHE_EXIST_DIR))
   {
      i = strlen(szPathname);
      if (i > 1 && i < LNSZ)
      {
         strcpy(szSubPath, szPathname);
         p = szSubPath + i - 1;
         i--;   // To skip the last '/'
         do
         {
            i--;
            p--;
         }
         while (*p != '/' && i) ;

         if (*p == '/')
         {
            p++;
            *p = 0;
            
            iErr = MakePath(szSubPath);
            if (!iErr)
            {
               if (mkdir(szPathname, 0775))
                  iErr = ERROR_PKGCACHE_ACCESS;
            }
         }
         else
            iErr = ERROR_PKGCACHE_ACCESS;
      }
      else if (i)
         iErr = ERROR_PKGCACHE_ACCESS;
   }

   return(iErr);
}


/*
 *  StrnCopy
 */

void
StrnCopy(char *dst, const char *src, const int l)
{
   if (l > 1)
   {
      strncpy(dst, src, l-1);
      dst[l-1] = 0;
   }
   else
      *dst = 0;
}

