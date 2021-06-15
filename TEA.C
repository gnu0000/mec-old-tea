/*
 * tea.c
 * Saturday, 2/25/1995.
 *
 */

#define INCL_VIO
#define INCL_DOSMISC
#include <os2.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <GnuArg.h>
#include <GnuMem.h>
#include <GnuZip.h>
#include <GnuScr.h>
#include <GnuKbd.h>
#include <GnuRes.h>
#include <GnuMisc.h>
#include <GnuStr.h>

/*---- this line is a test that is used because it is in this source file which is included in the resource data, and is too long to display wthout wrapping or at least scrolling to the appropriate length to view ---*/


#define MAX_VIEWLINES 15000
#define MAX_VIEWSIZE  32768U
#define MAX_DESCRLEN  56
#define MAX_BOOKMARKS 9

#define fWRAP         1   // spliff option
#define fKILLBLANKS   2   // spliff option
#define fLEFTJUST     3   // spliff option
#define fREFORMATLINE 4   // buffer mod
#define fREFORMATBODY 5   // buffer mod

typedef struct
   {
   FLAG   fFlags;
   USHORT uBookMark[MAX_BOOKMARKS];
   PSZ    psz;
   } FO;
typedef FO *PFO;


/*
 * Strings from TeaTxt.DAT
 *
 */
extern char szUsage       [];
extern char szHelpMain    [];
extern char szHelpAbout   [];
extern char szHelpCmdLine [];
extern char szHelpCreating[];
extern char szHelpListKeys[];
extern char szHelpViewKeys[];


PGW pgwLIST = NULL;
PGW pgwVIEW = NULL;
PGW pgwTMP;

PSZ  pszRESFILE;
FILE *fpRES; 

char szDESCFILE  [128];
char szCOMPBUFF  [36000];

char szVIEWHEADER [80];
char szVIEWFOOTER [] = "[<F1>-Help <ESC>-List]";

char szLISTHEADER [] = "[Resource List]";
char szLISTFOOTER [] = "[<ENTER>-Select <ESC>-Exit]";

USHORT uWRAPPOS = 72;
USHORT uHSCROLL = 0;

#define MAX_MODES 4
USHORT uMODE = 0;
USHORT MODES[MAX_MODES] = {25, 28, 43, 50};
USHORT uDATABLOCK = 1;

BOOL   bLINENUMBERS = FALSE;

USHORT uMAXBUFFERS;
PSZ pszBuffers[20];
USHORT uACTIVEBUFFERS = 0;


char   szSEARCH [128] = "";
BOOL   bSEARCHCASE    = FALSE;
BOOL   bHILITESEARCH  = FALSE;
USHORT uSEARCHLEN     = 0;
USHORT uSEARCHSTART   = 0;

/*************************************************************************/
/*                                                                       */
/*                                                                       */
/*                                                                       */
/*************************************************************************/

void Usage (void)
   {
   printf (szUsage, "1.0", __DATE__);
   exit (0);
   }


USHORT _cdecl Help (USHORT c)
   {
   PGW    pgw1;

   pgw1 = GnuCreateWin (9, 56, NULL);
   pgw1->pszHeader = "[Help]";
   pgw1->pszFooter = "[<ESC> to Exit Help]";
   GnuPaintBorder (pgw1);
   GnuPaintBig (pgw1, 1, 3, 8, 50, 0, 0, szHelpMain);

   while (TRUE)
      {
      switch (c = KeyChoose ("\x1B", "\x3B\x3C\x3D\x3E\x3F"))
         {
         case 0x13B: 
            GnuMsgBox ("[Help - About]", "[<ESC> to continue]", "\x1B\x0D", szHelpAbout);
            break;
         case 0x13C: 
            GnuMsgBox ("[Help - Command Line]", "[<ESC> to continue]", "\x1B\x0D", szUsage);
            break;
         case 0x13D: 
            GnuMsgBox ("[Help - Creating Resources]", "[<ESC> to continue]", "\x1B\x0D", szHelpCreating);
            break;
         case 0x13E: 
            GnuMsgBox ("[Help - List Commands]", "[<ESC> to continue]", "\x1B\x0D", szHelpListKeys);
            break;
         case 0x13F: 
            GnuMsgBox ("[Help - View Commands]", "[<ESC> to continue]", "\x1B\x0D", szHelpViewKeys);
            break;
         case 0x1B:
            GnuDestroyWin (pgw1);
            return 0;
         }
      }
   }





void SayWorking (void)
   {
   pgwTMP = GnuCreateWin (5, 20, NULL);
   GnuPaint (pgwTMP, 1, 0, 3, 0, "Working ...");
   }


void DoneWorking (void)
   {
   GnuDestroyWin (pgwTMP);
   }


USHORT MyErr (PSZ psz1, PSZ psz2)
   {
   return GnuMsg ("[Error]", psz1, psz2);
   }





/*************************************************************************/
/*                                                                       */
/*                                                                       */
/*                                                                       */
/*************************************************************************/


USHORT Spliff (PSZ pszBuff, USHORT uBuffLen, PSZ *ppsz, USHORT uFlags, USHORT uMaxLines)
   {
   USHORT uLineLen, uLine, uLen, uWrap;
   PSZ    p, pprev, psz;

   ppsz[0] = pszBuff;

   uWrap = (IsFlag (uFlags, fWRAP) ? uWRAPPOS : 32767);

   for (uLineLen=uLine=uLen=0, p=pprev=pszBuff; (uLen<uBuffLen)&&(uLine+1<uMaxLines); uLen++, p++)
      {
      if (!*p || *p == '\n' || ((uLineLen > uWrap) && *p == ' '))
         {                           
         *p = '\0';

         pprev = ppsz[uLine];
         if (IsFlag (uFlags, fKILLBLANKS) && StrBlankLine (pprev))
            {
            ppsz[uLine] = p+1;
            }
         else
            {
            if (IsFlag (uFlags, fLEFTJUST))
               {
               for (psz = ppsz[uLine]; *psz == ' ' || *psz == '\t'; psz++)
                  ;
               ppsz[uLine] = psz;
               }
            ppsz[++uLine] = p+1;
            }

         uLineLen = 0;
         continue;
         }
      uLineLen++;
      }
   return uLine;
   }


/**************************************************************************/
/*                                                                        */
/*                                                                        */
/*                                                                        */
/**************************************************************************/



BOOL LoadView (USHORT uIndex, BOOL bLoadBuff)
   {
   PRES   pres;
   PFO    pfo;
   PSZ    *ppsz;
   USHORT uLen, uLines, i;
   FILE   *fp;
   ULONG  ulRead = 0;

   pres = (PRES) pgwLIST->pUser1 + uIndex;
   pfo  = (PFO)  pgwLIST->pUser2;
   ppsz = (PSZ *)pgwVIEW->pUser1;

   if (!(fp = ResGetPtr (pszRESFILE, (PSZ)(ULONG)uIndex, NULL)))
      return FALSE;

   SayWorking ();


   /*---- possibly skip some buffers ---*/
   for (i=0; i<uDATABLOCK-1 && pres->ulOrgLen-ulRead > (ULONG)(uACTIVEBUFFERS) * (ULONG)MAX_VIEWSIZE; i++)
      {
      if (pres->uCompression)
         Cmp2fpEsz (pszBuffers[0], MAX_VIEWSIZE, &uLen, fp);
      else
         uLen = fread (pszBuffers[0], 1, 32767, fp);
      ulRead += (ULONG) uLen;
      }
   uDATABLOCK = i + 1;

   /*--- always re-load buffer for now ---*/
   for (uLines=i=0; i<uACTIVEBUFFERS && ulRead<pres->ulOrgLen; i++)
      {
      if (pres->uCompression)
         Cmp2fpEsz (pszBuffers[i], MAX_VIEWSIZE, &uLen, fp);
      else
         uLen = fread (pszBuffers[i], 1, 32767, fp);
      ulRead += (ULONG) uLen;

      uLines += Spliff (pszBuffers[i], uLen, ppsz+uLines, pfo[uIndex].fFlags, MAX_VIEWLINES-uLines);
      }
   fclose (fp);
   pgwVIEW->uItemCount = uLines;

   if (ulRead < pres->ulOrgLen)
      sprintf (szVIEWHEADER, "[%3.3u  %s   %4.4u Lines   %5.5lu of %5.5lu Bytes]",
                  uIndex, pres->szName, pgwVIEW->uItemCount, ulRead, pres->ulOrgLen);
   else
      sprintf (szVIEWHEADER, "[%3.3u  %s   %4.4u Lines   %5.5lu Bytes]",
                  uIndex, pres->szName, pgwVIEW->uItemCount, pres->ulOrgLen);

   DoneWorking ();
   return TRUE;
   }


USHORT LoadResources (PSZ pszRes)
   {
   PRES   pres;
   PFO    pfo;
   USHORT uRecs, uRead;

   fpRES = ResFindRecords (pszRes, &uRecs);

   pgwLIST->pUser1 = pres = calloc (uRecs, sizeof (RES));
   pgwLIST->pUser2 = pfo  = calloc (uRecs, sizeof (FO));

   uRead = fread (pres, sizeof (RES), uRecs, fpRES);
   if (uRead != uRecs)
      MyErr ("Resources are Truncated or invalid", pszRes);

   pgwLIST->uSelection = 0;
   pgwLIST->uItemCount = uRecs;

   if (!uRecs)
      MyErr ("No Resources found", pszRes);

   fclose (fpRES);
   return uRecs;
   }



USHORT SaveDescriptions (PSZ pszFile)
   {
   PFO    pfo;
   FILE   *fp;
   USHORT i;

   if (!(fp = fopen (szDESCFILE, "wb")))
      return MyErr ("Cannot open file", pszFile);

   SayWorking ();
   pfo = (PFO) pgwLIST->pUser2;
   CmpInitIO (fp, szCOMPBUFF, NULL, 0);
   Cmpfwrite (&pgwLIST->ulAtts, sizeof (ULONG));
   Cmpfwrite (&pgwVIEW->ulAtts, sizeof (ULONG));
   for (i=0; i<pgwLIST->uItemCount; i++)
      {      
      Cmpfwrite (&pfo[i].fFlags, 2);
      Cmpfwrite (pfo[i].uBookMark, MAX_BOOKMARKS * sizeof (USHORT));
      Cmpfwrite (pfo[i].psz ? pfo[i].psz : "", 0);
      }
   CmpTermIO ();
   fclose (fp);
   DoneWorking ();
   return 0;
   }


USHORT LoadDescriptions (PSZ pszFile)
   {
   PFO    pfo;
   FILE   *fp;
   USHORT i;
   RES    res;
   PSZ    psz, pszResName;

   if (!(fp = fopen (pszFile, "rb")))
      {
      pszResName = ((psz = strrchr (pszFile, '\\')) ? psz+1: pszFile);
      if (!(fp = ResGetPtr (pszRESFILE, pszResName, &res)))
         return MyErr ("Cannot find/open descriptions file", pszFile);
      if (res.uCompression)
         {
         fclose (fp);
         return MyErr ("Cannot read a compressed description resource", pszFile);
         }
      }

   SayWorking ();
   pfo = (PFO) pgwLIST->pUser2;
   for (i=0; i<pgwLIST->uItemCount; i++)
      pfo[i].psz = MemFreeData (pfo[i].psz);

   CmpInitIO (fp, szCOMPBUFF, NULL, 0);
   Cmpfread (&pgwLIST->ulAtts, sizeof (ULONG));
   Cmpfread (&pgwVIEW->ulAtts, sizeof (ULONG));
   for (i=0; i<pgwLIST->uItemCount; i++)
      {
      Cmpfread (&pfo[i].fFlags, 2);
      Cmpfread (pfo[i].uBookMark, MAX_BOOKMARKS * sizeof (USHORT));
      pfo[i].psz = Cmpfread (NULL, 0);
      }
   CmpTermIO ();
   fclose (fp);
   DoneWorking ();
   return 0;
   }


/*
 * uses: szSEARCH and bSEARCHCASE
 *
 *
 */
BOOL LineMatch (PSZ psz, PUSHORT puCol)
   {
   USHORT i, c1, c2;

   if (!*szSEARCH)
      return FALSE;

   c1 = (bSEARCHCASE ? (USHORT)*szSEARCH : toupper (*szSEARCH));

   for (i=0; *psz; psz++, i++)
      {
      c2 = (bSEARCHCASE ? (USHORT)*psz : toupper (*psz));
      if (c1 != c2)
         continue;
      if (bSEARCHCASE)
         {
         if (strnicmp (szSEARCH, psz, uSEARCHLEN))
            continue;
         }
      else
         {
         if (strncmp (szSEARCH, psz, uSEARCHLEN))
            continue;
         }
      if (puCol)
         *puCol = i;
      return TRUE;
      }
   return FALSE;
   }





USHORT _cdecl pfnPaintView (PGW pgw, USHORT uIndex, USHORT uLine)
   {
   PSZ    psz, *ppszStr;
   USHORT uAtt, uStrLen, uLen = 0, uOffset, uStrOffset=0, uLineStart, uCol;
   char   szTmp[8];

   if (!pgw->pUser1 || uIndex >= pgw->uItemCount)
      return 0;

   uAtt = (pgw->uSelection == uIndex ? 2 : 1);
   ppszStr = pgw->pUser1;

   if ((uOffset = bLINENUMBERS * 5) && !uHSCROLL)
      {
      sprintf (szTmp, "%4.4u ", uIndex);
      uLen = GnuPaint (pgw, uLine, 0, 0, uAtt, szTmp);
      }

   psz = ppszStr[uIndex];
   if (uHSCROLL && psz && *psz)
      {
      uStrLen = strlen (psz);
      uStrOffset = (uStrLen + uOffset > uHSCROLL ? uHSCROLL - uOffset: uStrLen);
      psz += uStrOffset;
      }
   uLineStart = uLen;
   uLen += GnuPaint2 (pgw, uLine, uLineStart, 0, uAtt, psz, pgw->uClientXSize- uLen);

   /*--- blank off rest of line ---*/
   if (uLen < pgw->uClientXSize)
      GnuPaintNChar (pgw, uLine, uLen, 0, uAtt, ' ', pgw->uClientXSize - uLen);

   /*--- search hilite? ---*/
   if (!bHILITESEARCH)
      return 1;
   if (!LineMatch (psz, &uCol))
      return 1;
   if ((uStrOffset > uCol))      // off to left
      return 1;
   if (uCol - uStrOffset > pgw->uClientXSize - uLineStart) // off to right
      return 1;
   uLen += GnuPaint (pgw, uLine, uCol-uStrOffset+uLineStart, 0, 2, szSEARCH);
   return 1;
   }



USHORT _cdecl pfnPaintList (PGW pgw, USHORT uIndex, USHORT uLine)
   {
   PRES   pres;
   PFO    pfo;
   USHORT uLen;
   USHORT uAtt;
   char   szTmp[8];

   if (!pgw->pUser1 || uIndex >= pgw->uItemCount)
      return 0;

   pres = (PRES) pgw->pUser1 + uIndex;
   pfo  = (PFO)  pgw->pUser2;

   uAtt = (pgw->uSelection == uIndex ? 2 : 1);

   sprintf (szTmp, " %3.3d ", uIndex);
   uLen  = GnuPaint2 (pgw, uLine, 0, 0, uAtt, szTmp, 5);

   uLen += GnuPaint2 (pgw, uLine, uLen, 0, uAtt, pres->szName, 16);
   uLen += GnuPaintNChar (pgw, uLine, uLen, 0, uAtt, ' ', 22 - uLen);

   if (pfo[uIndex].psz)
      uLen += GnuPaint2 (pgw, uLine, uLen, 0, uAtt, pfo[uIndex].psz, MAX_DESCRLEN);

   /*--- blank off rest of line ---*/
   if (uLen < pgw->uClientXSize)
      GnuPaintNChar (pgw, uLine, uLen, 0, uAtt, ' ', pgw->uClientXSize - uLen);
   return 1;
   }



USHORT CreateWindows (BOOL bShowList)
   {
   PMET   pmet;
   PVOID  pL1, pL2, pV1, pV2;
   USHORT uL1, uV1, uV2;

   pL1 = (pgwLIST ? pgwLIST->pUser1     : NULL);
   pL2 = (pgwLIST ? pgwLIST->pUser2     : NULL);
   uL1 = (pgwLIST ? pgwLIST->uItemCount : 0);
   pV1 = (pgwVIEW ? pgwVIEW->pUser1     : malloc (MAX_VIEWLINES * sizeof (PSZ)));
   pV2 = (pgwVIEW ? pgwVIEW->pUser2     : NULL);
   uV1 = (pgwVIEW ? pgwVIEW->uItemCount : 0);
   uV2 = (pgwVIEW ? pgwVIEW->uScrollPos : 0);

   if (pgwLIST)  // windows already exist, this is a change-mode
      {
      GnuDestroyWin (pgwLIST);
      GnuDestroyWin (pgwVIEW);
      }

   pmet = ScrGetMetrics ();
   ScrSetIntenseBkg (TRUE);
   GnuPaintAtCreate (FALSE);

   pgwVIEW = GnuCreateWin (pmet->uYSize, pmet->uXSize, pfnPaintView);
   gnuFreeDat (pgwVIEW);
   pgwVIEW->bShowScrollPos = TRUE;
   pgwVIEW->pszHeader  = szVIEWHEADER;
   pgwVIEW->pszFooter  = szVIEWFOOTER;
   pgwVIEW->bShadow    = FALSE;
   pgwVIEW->pUser1     = pV1;
   pgwVIEW->pUser2     = pV2;
   pgwVIEW->uItemCount = uV1;
   pgwVIEW->uScrollPos = uV2;

   pgwLIST = GnuCreateWin (pmet->uYSize, pmet->uXSize, pfnPaintList);
   gnuFreeDat (pgwLIST);
   pgwLIST->bShowScrollPos = TRUE;
   pgwLIST->pszHeader  = szLISTHEADER;
   pgwLIST->pszFooter  = szLISTFOOTER;
   pgwLIST->bShadow    = FALSE;
   pgwLIST->pUser1     = pL1;
   pgwLIST->pUser2     = pL2;
   pgwLIST->uItemCount = uL1;
   pgwLIST->uSelection = 0;

   GnuPaintAtCreate (TRUE);
   GnuPaintWin ((bShowList ? pgwLIST : pgwVIEW), 0xFFFF);
   GnuPaintBorder ((bShowList ? pgwLIST : pgwVIEW));
   return 0;
   }


USHORT Cleanup (void)
   {
   PMET   pmet;

   pmet = ScrGetMetrics ();

   GnuDestroyWin (pgwVIEW);
   GnuDestroyWin (pgwLIST);

   ScrRestoreMode ();
   VioScrollDn (0, 0, 0xFFFF, 0xFFFF, 0xFFFF, pmet->bcOriginal, 0);
   GnuMoveCursor (NULL, 0, 0);
   ScrShowCursor (TRUE);
   return 0;
   }


/*************************************************************************/
/*                                                                       */
/*                                                                       */
/*                                                                       */
/*************************************************************************/



USHORT EditDesc (USHORT uIndex)
   {
   PGW    pgw;
   char   szTmp[128];
   PFO    pfo;
   PRES   pres;
   USHORT uRet;

   pres = (PRES)pgwLIST->pUser1 + uIndex;
   pfo  = (PFO) pgwLIST->pUser2;

   ScrSaveCursor (TRUE);
   pgw = GnuCreateWin (8, 70, NULL);
   pgw->pszHeader = "[Edit Resource]";
   pgw->pszFooter = "[Press <ESC> to cancel]";
   GnuPaintBorder (pgw);

   GnuPaintBig (pgw, 1, 1, 4, 7, 1, 0, "Index:\nName:\nFlags:\nDesc:\n");

   sprintf (szTmp, "%3.3d", uIndex);
   GnuPaint (pgw, 1, 9, 0, 0, szTmp);
   GnuPaint (pgw, 2, 9, 0, 0, pres->szName);
   sprintf (szTmp, "%4.4x", pfo[uIndex].fFlags);
   GnuPaint (pgw, 3, 9, 0, 0, szTmp);
   GnuPaintNChar (pgw, 4, 9, 0, 1, ' ', MAX_DESCRLEN);
   strcpy (szTmp, (pfo[uIndex].psz ? pfo[uIndex].psz : ""));

   KeyEditCellMode (NULL, "\x50\x48", 2);
   if (uRet=KeyEditCell (szTmp, TopOf(pgw)+5, LeftOf(pgw)+10, MAX_DESCRLEN, 2))
      {
      MemFreeData (pfo[uIndex].psz);
      pfo[uIndex].psz = strdup (szTmp);
      }
   GnuDestroyWin (pgw);
   ScrRestoreCursor ();
   return (uRet > 0x100 ? uRet : 0);  // only pass back the extended keys
   }


USHORT JumpToBookmark (USHORT uIndex)
   {
   PGW    pgw;
   PFO    pfo;
   USHORT c;

   pfo  = (PFO) pgwLIST->pUser2;

   pgw = GnuCreateWin (5, 28, NULL);
   pgw->pszFooter = "[<ESC>-Cancel]";
   GnuPaint (pgw, 1, 2, 0, 0, "Enter Bookmark (0-9)");
   c = KeyChoose ("0123456789\x1B", NULL);
   GnuDestroyWin (pgw);
   if (!c || c == 0x1B)
      return 0;

   GnuScrollTo (pgwVIEW, pfo[uIndex].uBookMark[c - '1']);
   return 0;
   }



//for (i=0; i<pgwVIEW->uItemCount; i++)
//fprintf (fp, "%s\n", ppsz[i]);
BOOL WriteBuffer (USHORT uIndex)
   {
   PGW    pgw;
   char   szTmp [128];
   PRES   pres;
   USHORT i, uSize;
   FILE   *fp, *fpRes;
   PSZ    *ppsz;
   ULONG  ulRead;

   pres = (PRES) pgwLIST->pUser1 + uIndex;
   ppsz = (PSZ *)pgwVIEW->pUser1;

   ScrSaveCursor (TRUE);
   pgw = GnuCreateWin (7, 70, NULL);
   pgw->pszHeader = "[Save Resource]";
   pgw->pszFooter = "[Press <ESC> to cancel]";
   GnuPaintBorder (pgw);

   GnuPaint (pgw, 1, 1, 0, 0, "Enter Save Name:");
   strcpy (szTmp, pres->szName);
   if (KeyEditCell (szTmp, TopOf(pgw)+4, LeftOf(pgw)+3, MAX_DESCRLEN, 2))
      {
      if (!(fp = fopen (szTmp, "wt")))
         MyErr ("Unable to open file", szTmp);
      else
         {
         fpRes = ResGetPtr (pszRESFILE, (PSZ)(ULONG)uIndex, NULL);
         for (i=0,ulRead=0; ulRead<pres->ulOrgLen; i++)
            {
            Cmp2fpEfp (fp, &uSize, fpRes, NULL);
            ulRead += uSize;
            }
         fclose (fpRes);
         fclose (fp);
         }
      }
   GnuDestroyWin (pgw);
   ScrRestoreCursor ();
   return TRUE;
   }



BOOL Search (BOOL bCase, BOOL bNext)
   {
   PGW    pgw;
   USHORT i, uRet;
   PSZ    *ppsz;

   bSEARCHCASE = bCase;
   if (!bNext)
      {
      uSEARCHSTART = 0;
      ScrSaveCursor (TRUE);
      pgw = GnuCreateWin (7, 70, NULL);
      pgw->pszHeader = "[Find Text]";
      pgw->pszFooter = "[Press <ESC> to cancel]";
      GnuPaintBorder (pgw);

      GnuPaint (pgw, 1, 1, 0, 0, "Enter Search String:");
      uRet = KeyEditCell (szSEARCH, TopOf(pgw)+4, LeftOf(pgw)+3, MAX_DESCRLEN, 2);
      GnuDestroyWin (pgw);
      ScrRestoreCursor ();
      if (!uRet)
         return FALSE;

      uSEARCHLEN = strlen (szSEARCH);
      }
   ppsz = pgwVIEW->pUser1;
   for (i=uSEARCHSTART; i<pgwVIEW->uItemCount; i++)
      if (LineMatch (ppsz[i], NULL))
         {
         GnuMakeVisible (pgwVIEW, i);
         bHILITESEARCH = TRUE;
         GnuPaintWin (pgwVIEW, i);
         bHILITESEARCH = FALSE;
         uSEARCHSTART = i+1;
         return TRUE;
         }
   return FALSE;
   }



USHORT DoViewWindow (USHORT uIndex)
   {
   USHORT c;
   PFO    pfo;
   char   szTmp[16];

   uDATABLOCK=1;
   LoadView (uIndex, TRUE);
   uHSCROLL = 0;
   pgwVIEW->uScrollPos = 0;

   GnuPaintBorder (pgwVIEW);
   GnuPaintWin (pgwVIEW, 0xFFFF);

   pfo  = (PFO) pgwLIST->pUser2;

   while (TRUE)
      {
      sprintf (szTmp, "[%4.4u]", pgwVIEW->uScrollPos);
      GnuPaint (NULL, 0, 3, 0, 0, szTmp);

      c = KeyGet (TRUE);
      if (GnuDoListKeys (pgwVIEW, c))
         continue;

      switch (c)
         {
         case 0x14d:  // right                    H Scroll left
            uHSCROLL += 10;
            GnuPaintWin (pgwVIEW, 0xFFFF);
            break;

         case 0x14B:  // left                     H Scroll right
            if (!uHSCROLL)
               break;
            uHSCROLL -= min (uHSCROLL, 10);
            GnuPaintWin (pgwVIEW, 0xFFFF);
            break;

         case 0x173:  // ctl-left                 next data block
            if (uDATABLOCK < 2)
               break;
            uDATABLOCK--;
            LoadView (uIndex, TRUE);
            uHSCROLL = 0;
            GnuPaintBorder (pgwVIEW);
            GnuPaintWin (pgwVIEW, 0xFFFF);
            GnuMakeVisible (pgwVIEW, pgwVIEW->uSelection);
            break;

         case 0x174:  // ctl-right                prev data block
            uDATABLOCK++;
            LoadView (uIndex, TRUE);
            uHSCROLL = 0;
            GnuPaintBorder (pgwVIEW);
            GnuPaintWin (pgwVIEW, 0xFFFF);
            GnuMakeVisible (pgwVIEW, pgwVIEW->uSelection);
            break;

//         case 0x173:  // ctl-left                 H Scroll to start
//            if (!uHSCROLL)
//               break;
//            uHSCROLL = 0;
//            GnuPaintWin (pgwVIEW, 0xFFFF);
//            break;

         case 'S' :  // S           save descriptions
            SaveDescriptions (szDESCFILE);
            break;

         case 'L' :  // L           load descriptions
            LoadDescriptions (szDESCFILE);
            GnuPaintWin (pgwLIST, 0xFFFF);
            break;

         case 'E' :  // E                         edit descriptions
         case 'C' :  // C          
         case 'D' :  // D
            EditDesc (pgwLIST->uSelection);
            break;

         case 'W' :  // W
            WriteBuffer (uIndex);
            break;

         case 0x176: // Ctl-pg dn                  next buff
            uIndex = (uIndex + 1) % pgwLIST->uItemCount;
            uDATABLOCK=1;
            LoadView (uIndex, TRUE);
            uHSCROLL = 0;
            pgwVIEW->uScrollPos = 0;
            GnuPaintBorder (pgwVIEW);
            GnuPaintWin (pgwVIEW, 0xFFFF);
            break;

         case 0x184: // Ctl-pg up                  prev buff
            uIndex = (uIndex ? uIndex - 1 : pgwLIST->uItemCount - 1);
            uDATABLOCK=1;
            LoadView (uIndex, TRUE);
            uHSCROLL = 0;
            pgwVIEW->uScrollPos = 0;
            GnuPaintBorder (pgwVIEW);
            GnuPaintWin (pgwVIEW, 0xFFFF);
            break;

         case 0x1B:  // esc
            return 0;

         case 0x111: // Alt-W                      wrap
            SetFlag (&pfo[uIndex].fFlags, fWRAP, 2);
            LoadView (uIndex, !IsFlag (pfo[uIndex].fFlags, fWRAP));
            GnuPaintBorder (pgwVIEW);
            GnuPaintWin (pgwVIEW, 0xFFFF);
            GnuMakeVisible (pgwVIEW, pgwVIEW->uSelection);
            break;

         case 0x125: // Alt-K                      kill blanks
            SetFlag (&pfo[uIndex].fFlags, fKILLBLANKS, 2);
            LoadView (uIndex, FALSE);
            GnuPaintBorder (pgwVIEW);
            GnuPaintWin (pgwVIEW, 0xFFFF);
            GnuMakeVisible (pgwVIEW, pgwVIEW->uSelection);
            break;

         case 0x126: // Alt-L                      left just
            SetFlag (&pfo[uIndex].fFlags, fLEFTJUST, 2);
            LoadView (uIndex, FALSE);
            GnuPaintBorder (pgwVIEW);
            GnuPaintWin (pgwVIEW, 0xFFFF);
            GnuMakeVisible (pgwVIEW, pgwVIEW->uSelection);
            break;

         case 0x124:  // Alt-J                      jump to bookmark
            JumpToBookmark (uIndex);
            break;

         case 0x132:  // Alt-M                      screen mode
            uMODE = (uMODE + 1) % MAX_MODES;
            ScrSetMode (MODES[uMODE], 80); 
            CreateWindows (FALSE); 
            GnuPaintWin (pgwVIEW, 0xFFFF);
            break;

         case 0x131:  // Alt-N                      line numbers
            bLINENUMBERS = !bLINENUMBERS;
            GnuPaintWin (pgwVIEW, 0xFFFF);
            break;

         case 0x12E: // Alt-C        set colors
         case 0x118: // Alt-O
            GnuSetColorWindow (pgwVIEW);
            GnuPaintBorder (pgwVIEW);
            GnuPaintWin (pgwVIEW, 0xFFFF);
            break;

         case 0x11F: // Alt-S
         case '\\':  // 
            Search (FALSE, FALSE);
            break;

         case '/':   // 
            Search (TRUE, FALSE);
            break;

         case 0x158: // Shift-F5:
            Search (bSEARCHCASE, TRUE);
            break;

         default:
            if (c >= 0x178 && c <= 0x180)
               pfo[uIndex].uBookMark[c - 0x178] = pgwVIEW->uScrollPos;
            else
               Beep (0);
         }
      }
   }


/*************************************************************************/
/*                                                                       */
/*                                                                       */
/*                                                                       */
/*************************************************************************/

USHORT DoListWindow (void)
   {
   USHORT c, uNext = 0;;

   GnuPaintBorder (pgwLIST);
   GnuPaintWin (pgwLIST, 0xFFFF);

   while (TRUE)
      {
      c = (uNext ? uNext : KeyGet (TRUE));
      uNext = (uNext > 0x100 ? 'C' : 0);
      if (GnuDoListKeys (pgwLIST, c))
         continue;

      switch (c)
         {
         case 0x0D:  // enter       view
         case 'V' :  // V
            DoViewWindow (pgwLIST->uSelection);
            GnuPaintBorder (pgwLIST);
            GnuPaintWin (pgwLIST, 0xFFFF);
            break;

         case 'S' :  // S           save descriptions
            SaveDescriptions (szDESCFILE);
            break;

         case 'L' :  // L           load descriptions
            LoadDescriptions (szDESCFILE);
            GnuPaintWin (pgwLIST, 0xFFFF);
            break;

         case 'E' :  // E           edit descriptions
         case 'C' :  // C          
         case 'D' :  // D
            uNext = EditDesc (pgwLIST->uSelection);
            GnuPaintWin (pgwLIST, pgwLIST->uSelection);
            break;

         case 'R' :  // R            random file
            GnuSelectLine (pgwLIST, rand() % pgwLIST->uItemCount , TRUE);
            break;

         case 0x12E: // Alt-C        set colors
         case 0x118: // Alt-O
            GnuSetColorWindow (pgwLIST);
            GnuPaintBorder (pgwLIST);
            GnuPaintWin (pgwLIST, 0xFFFF);
            break;

         case 0x1B:  // esc
            return 0;

         case 0x132:  // Alt-M
            uMODE = (uMODE + 1) % MAX_MODES;
            ScrSetMode (MODES[uMODE], 80); 
            CreateWindows (FALSE); 
            GnuPaintWin (pgwLIST, 0xFFFF);
            break;

         default:
            Beep (0);
         }
      }
   }


void Welcome (void)
   {
   USHORT c;
   PGW pgw;
   pgw = GnuCreateWin (12, 56, NULL);
   pgw->pszHeader = "[Error]";
   pgw->pszFooter = "[<ESC> to Exit]";
   GnuPaintBorder (pgw);
   GnuPaint (pgw, 1, 3, 0, 0,"uResIdx index is 0");
   c = KeyChoose ("\x1B", "\x78");
   if (c != 0x178)
      exit (1);
   GnuDestroyWin (pgw);
   }

/*************************************************************************/
/*                                                                       */
/*                                                                       */
/*                                                                       */
/*************************************************************************/

int _cdecl main (int argc, char *argv[])
   {
   PSZ    psz;
   USHORT i;
   BYTE   bMode;

   if (ArgBuildBlk ("? *^Help *^Resource% *^Buffers%"))
      Error ("%s", ArgGetErr ());

   if (ArgFillBlk (argv))
      Error ("%s", ArgGetErr ());

   if (ArgIs ("?") || ArgIs ("Help"))
      Usage ();

   if (ArgIs ("Buffers"))
      uMAXBUFFERS = atoi (ArgGet ("Buffers", 0));
   else
      {
      DosGetMachineMode (&bMode);
      uMAXBUFFERS =  (bMode == MODE_REAL ? 8 : 12);
      }

   pszRESFILE = (ArgIs ("Resource") ? ArgGet ("Resource", 0) : argv[0]);
   if (psz = strrchr (strcpy (szDESCFILE, pszRESFILE), '.'))
      *psz = '\0';
   strcat (szDESCFILE, ".DSC");

   for (i=uACTIVEBUFFERS; uACTIVEBUFFERS<uMAXBUFFERS; uACTIVEBUFFERS++)
      if (!(pszBuffers[uACTIVEBUFFERS] = malloc (MAX_VIEWSIZE)))
         break;

   ScrInitMetrics ();

   KeyAddTrap (0x13b, Help);
   Cmp2Init (szCOMPBUFF, 3, 2);

   ScrShowCursor (FALSE);
   CreateWindows (TRUE);
   LoadResources (pszRESFILE);
   LoadDescriptions (szDESCFILE);

// Welcome ();

   DoListWindow ();
   Cleanup ();
   return 0;
   }

