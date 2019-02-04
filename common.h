/* 
 * File:    common.h
 * 
 * Author:  fossette
 * 
 * Date:    2019/02/04
 *
 * Version: 1.1
 * 
 * Descr:   Common header file. Tested under FreeBSD 11.2.
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


#ifndef PKGCACHE_COMMON_H
#define PKGCACHE_COMMON_H


/*
 *  Constants
 */

#define LNFILENAME               1028
#define LNSZ                     500

#define PKGCACHE_EXIST_ANY       0
#define PKGCACHE_EXIST_FILE      1
#define PKGCACHE_EXIST_DIR       2

#define ERROR_PKGCACHE_ACCESS    1
#define ERROR_PKGCACHE_CMD       2
#define ERROR_PKGCACHE_FILE_R    3
#define ERROR_PKGCACHE_FILE_W    4
#define ERROR_PKGCACHE_INFO      5
#define ERROR_PKGCACHE_MEM       6
#define ERROR_PKGCACHE_NO_EOH    7
#define ERROR_PKGCACHE_REPO      8
#define ERROR_PKGCACHE_TEMP      9

// Remove comment to PKGCACHE_VERBOSE to have verbose debug output
// #define PKGCACHE_VERBOSE         1


/*
 *  Types
 */

typedef char FILENAME[LNFILENAME];


/*
 *  Prototypes
 */

int  Exist(const char *szPathname, const int iPathType);
int  MakePath(const char *szPathname);
void StrnCopy(char *dst, const char *src, const int l);
void StrReplace(char *dst, const char *before, const char after);


#endif  // PKGCACHE_COMMON_H
