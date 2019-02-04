/* 
 * File:    list.h
 * 
 * Author:  fossette
 * 
 * Date:    2019/02/04
 *
 * Version: 1.1
 * 
 * Descr:   Package list object header file. Tested under FreeBSD 11.2.
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


#ifndef PKGCACHE_LIST_H
#define PKGCACHE_LIST_H


/*
 *  Prototypes
 */

int  ListAdd(char *szPkgName);
int  ListGetFirst(     char *szPkgName);
void ListGetNavFilter(     char *pNavFilter);
int  ListGetNext(     char *szPkgName);
void ListGetPathFilter(     char *pPathFilter);
void ListGetRepoUrl(     char *pRepoUrl);
int  ListGetStatExisting(void);
int  ListGetStatNew(void);
int  ListIsFound(char *szPkgName);
int  ListLoad(char *szFilename);
void ListQuit(void);
int  ListSave(char *szFilename);


#endif  // PKGCACHE_LIST_H
