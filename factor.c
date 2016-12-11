/*
This file is part of Alpertron Calculators.

Copyright 2015 Dario Alejandro Alpern

Alpertron Calculators is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Alpertron Calculators is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Alpertron Calculators.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "bignbr.h"
#include "expression.h"
#include "factor.h"

#ifdef __EMSCRIPTEN__
void databack(char *data);
int stamp(void);
double tenths(void);
void GetDHMS(char **pptrText, int seconds);
int newStamp, oldStamp;
char upperText[30000];
char lowerText[30000];
char *ptrLowerText;
int yieldFreq;
mmCback modmultCallback;
extern int oldTimeElapsed;
extern double originalTenthSecond;
extern long long lModularMult;
#endif

#define TYP_AURIF  100000000
#define TYP_TABLE  150000000
#define TYP_SIQS   200000000
#define TYP_LEHMAN 250000000
#define TYP_RABIN  300000000
#define TYP_EC     350000000

#define SIEVE_SIZE (2*3*5*7)
#define HALF_SIEVE_SIZE (SIEVE_SIZE/2)
#define GROUP_SIZE ((2-1)*(3-1)*(5-1)*(7-1))

static int nbrPrimes, indexPrimes, StepECM;
static int FactorIndex;
static BigInteger power, prime;
int *factorArr[FACTOR_ARRSIZE];
static int Typ[4000];
static int indexM, maxIndexM;
static int foundByLehman, performLehman;
static int SmallPrime[670]; /* Primes < 5000 */
int NextEC;
static int EC;
static limb A0[MAX_LEN];
static limb A02[MAX_LEN];
static limb A03[MAX_LEN];
static limb AA[MAX_LEN];
static limb DX[MAX_LEN];
static limb DZ[MAX_LEN];
static limb GD[MAX_LEN];
static limb M[MAX_LEN];
static limb TX[MAX_LEN];
static limb TZ[MAX_LEN];
static limb UX[MAX_LEN];
static limb UZ[MAX_LEN];
static limb W1[MAX_LEN];
static limb W2[MAX_LEN];
static limb W3[MAX_LEN];
static limb W4[MAX_LEN];
static limb WX[MAX_LEN];
static limb WZ[MAX_LEN];
static limb X[MAX_LEN];
static limb Z[MAX_LEN];
static limb Aux1[MAX_LEN];
static limb Aux2[MAX_LEN];
static limb Aux3[MAX_LEN];
static limb Aux4[MAX_LEN];
static limb Xaux[MAX_LEN];
static limb Zaux[MAX_LEN];
static limb root[GROUP_SIZE][MAX_LEN];
static unsigned char sieve[10*SIEVE_SIZE];
static unsigned char sieve2310[SIEVE_SIZE];
static int sieveidx[GROUP_SIZE];
static limb GcdAccumulated[MAX_LEN];

static limb *fieldAA, *fieldTX, *fieldTZ, *fieldUX, *fieldUZ;
static void add3(limb *x3, limb *z3, limb *x2, limb *z2, limb *x1, limb *z1, limb *x, limb *z);
static void duplicate(limb *x2, limb *z2, limb *x1, limb *z1);
static limb *fieldAux1 = Aux1;
static limb *fieldAux2 = Aux2;
static limb *fieldAux3 = Aux3;
static limb *fieldAux4 = Aux4;
static BigInteger Temp1, Temp2, Temp3;
BigInteger factorValue, tofactor;
extern int groupLen;
#ifdef __EMSCRIPTEN__
extern char *ptrInputText;
#endif
enum eEcmResult
{
  FACTOR_NOT_FOUND = 0,
  FACTOR_FOUND,
  CHANGE_TO_SIQS,
};

/* ECM limits for 30, 35, ..., 85 digits */
static int limits[] = { 5, 5, 5, 5, 5, 6, 8, 10, 20, 25, 26, 30, 50 };
/******************************************************/
/* Start of code adapted from Paul Zimmermann's ECM4C */
/******************************************************/
#define ADD 6  /* number of multiplications in an addition */
#define DUP 5  /* number of multiplications in a duplicate */

#ifdef __EMSCRIPTEN__
static void GetYieldFrequency()
{
  yieldFreq = 1000000 / (NumberLength * NumberLength);
  if (yieldFreq > 100000)
  {
    yieldFreq = yieldFreq / 100000 * 100000;
  }
  else if (yieldFreq > 10000)
  {
    yieldFreq = yieldFreq / 10000 * 10000;
  }
  else if (yieldFreq > 1000)
  {
    yieldFreq = yieldFreq / 1000 * 1000;
  }
  else if (yieldFreq > 100)
  {
    yieldFreq = yieldFreq / 100 * 100;
  }
}
#endif
                          /* returns the number of modular multiplications */
static int lucas_cost(int n, double v)
{
  int c, d, e, r;

  d = n;
  r = (int)((double)d / v + 0.5);
  if (r >= n)
  {
    return (ADD * n);
  }
  d = n - r;
  e = 2 * r - n;
  c = DUP + ADD; /* initial duplicate and final addition */
  while (d != e)
  {
    if (d < e)
    {
      r = d;
      d = e;
      e = r;
    }
    if (4 * d <= 5 * e && ((d + e) % 3) == 0)
    { /* condition 1 */
      r = (2 * d - e) / 3;
      e = (2 * e - d) / 3;
      d = r;
      c += 3 * ADD; /* 3 additions */
    }
    else if (4 * d <= 5 * e && (d - e) % 6 == 0)
    { /* condition 2 */
      d = (d - e) / 2;
      c += ADD + DUP; /* one addition, one duplicate */
    }
    else if (d <= (4 * e))
    { /* condition 3 */
      d -= e;
      c += ADD; /* one addition */
    }
    else if ((d + e) % 2 == 0)
    { /* condition 4 */
      d = (d - e) / 2;
      c += ADD + DUP; /* one addition, one duplicate */
    }
    else if (d % 2 == 0)
    { /* condition 5 */
      d /= 2;
      c += ADD + DUP; /* one addition, one duplicate */
    }
    else if (d % 3 == 0)
    { /* condition 6 */
      d = d / 3 - e;
      c += 3 * ADD + DUP; /* three additions, one duplicate */
    }
    else if ((d + e) % 3 == 0)
    { /* condition 7 */
      d = (d - 2 * e) / 3;
      c += 3 * ADD + DUP; /* three additions, one duplicate */
    }
    else if ((d - e) % 3 == 0)
    { /* condition 8 */
      d = (d - e) / 3;
      c += 3 * ADD + DUP; /* three additions, one duplicate */
    }
    else if (e % 2 == 0)
    { /* condition 9 */
      e /= 2;
      c += ADD + DUP; /* one addition, one duplicate */
    }
  }
  return c;
}

/* computes nP from P=(x:z) and puts the result in (x:z). Assumes n>2. */
void prac(int n, limb *x, limb *z, limb *xT, limb *zT, limb *xT2, limb *zT2)
{
  int d, e, r, i;
  limb *t;
  limb *xA = x, *zA = z;
  limb *xB = fieldAux1, *zB = fieldAux2;
  limb *xC = fieldAux3, *zC = fieldAux4;
  double v[] =
  {
    1.61803398875,
    1.72360679775,
    1.618347119656,
    1.617914406529,
    1.612429949509,
    1.632839806089,
    1.620181980807,
    1.580178728295,
    1.617214616534,
    1.38196601125 };

  /* chooses the best value of v */
  r = lucas_cost(n, v[0]);
  i = 0;
  for (d = 1; d < 10; d++)
  {
    e = lucas_cost(n, v[d]);
    if (e < r)
    {
      r = e;
      i = d;
    }
  }
  d = n;
  r = (int)((double)d / v[i] + 0.5);
  /* first iteration always begins by Condition 3, then a swap */
  d = n - r;
  e = 2 * r - n;
  memcpy(xB, xA, NumberLength * sizeof(limb));   // B <- A
  memcpy(zB, zA, NumberLength * sizeof(limb));
  memcpy(xC, xA, NumberLength * sizeof(limb));   // C <- A
  memcpy(zC, zA, NumberLength * sizeof(limb));
  duplicate(xA, zA, xA, zA); /* A=2*A */
  while (d != e)
  {
    if (d < e)
    {
      r = d;
      d = e;
      e = r;
      t = xA;
      xA = xB;
      xB = t;
      t = zA;
      zA = zB;
      zB = t;
    }
    /* do the first line of Table 4 whose condition qualifies */
    if (4 * d <= 5 * e && ((d + e) % 3) == 0)
    { /* condition 1 */
      r = (2 * d - e) / 3;
      e = (2 * e - d) / 3;
      d = r;
      add3(xT, zT, xA, zA, xB, zB, xC, zC); /* T = f(A,B,C) */
      add3(xT2, zT2, xT, zT, xA, zA, xB, zB); /* T2 = f(T,A,B) */
      add3(xB, zB, xB, zB, xT, zT, xA, zA); /* B = f(B,T,A) */
      t = xA;
      xA = xT2;
      xT2 = t;
      t = zA;
      zA = zT2;
      zT2 = t; /* swap A and T2 */
    }
    else if (4 * d <= 5 * e && (d - e) % 6 == 0)
    { /* condition 2 */
      d = (d - e) / 2;
      add3(xB, zB, xA, zA, xB, zB, xC, zC); /* B = f(A,B,C) */
      duplicate(xA, zA, xA, zA); /* A = 2*A */
    }
    else if (d <= (4 * e))
    { /* condition 3 */
      d -= e;
      add3(xT, zT, xB, zB, xA, zA, xC, zC); /* T = f(B,A,C) */
      t = xB;
      xB = xT;
      xT = xC;
      xC = t;
      t = zB;
      zB = zT;
      zT = zC;
      zC = t; /* circular permutation (B,T,C) */
    }
    else if ((d + e) % 2 == 0)
    { /* condition 4 */
      d = (d - e) / 2;
      add3(xB, zB, xB, zB, xA, zA, xC, zC); /* B = f(B,A,C) */
      duplicate(xA, zA, xA, zA); /* A = 2*A */
    }
    else if (d % 2 == 0)
    { /* condition 5 */
      d /= 2;
      add3(xC, zC, xC, zC, xA, zA, xB, zB); /* C = f(C,A,B) */
      duplicate(xA, zA, xA, zA); /* A = 2*A */
    }
    else if (d % 3 == 0)
    { /* condition 6 */
      d = d / 3 - e;
      duplicate(xT, zT, xA, zA); /* T1 = 2*A */
      add3(xT2, zT2, xA, zA, xB, zB, xC, zC); /* T2 = f(A,B,C) */
      add3(xA, zA, xT, zT, xA, zA, xA, zA); /* A = f(T1,A,A) */
      add3(xT, zT, xT, zT, xT2, zT2, xC, zC); /* T1 = f(T1,T2,C) */
      t = xC;
      xC = xB;
      xB = xT;
      xT = t;
      t = zC;
      zC = zB;
      zB = zT;
      zT = t; /* circular permutation (C,B,T) */
    }
    else if ((d + e) % 3 == 0)
    { /* condition 7 */
      d = (d - 2 * e) / 3;
      add3(xT, zT, xA, zA, xB, zB, xC, zC); /* T1 = f(A,B,C) */
      add3(xB, zB, xT, zT, xA, zA, xB, zB); /* B = f(T1,A,B) */
      duplicate(xT, zT, xA, zA);
      add3(xA, zA, xA, zA, xT, zT, xA, zA); /* A = 3*A */
    }
    else if ((d - e) % 3 == 0)
    { /* condition 8 */
      d = (d - e) / 3;
      add3(xT, zT, xA, zA, xB, zB, xC, zC); /* T1 = f(A,B,C) */
      add3(xC, zC, xC, zC, xA, zA, xB, zB); /* C = f(A,C,B) */
      t = xB;
      xB = xT;
      xT = t;
      t = zB;
      zB = zT;
      zT = t; /* swap B and T */
      duplicate(xT, zT, xA, zA);
      add3(xA, zA, xA, zA, xT, zT, xA, zA); /* A = 3*A */
    }
    else if (e % 2 == 0)
    { /* condition 9 */
      e /= 2;
      add3(xC, zC, xC, zC, xB, zB, xA, zA); /* C = f(C,B,A) */
      duplicate(xB, zB, xB, zB); /* B = 2*B */
    }
  }
  add3(x, z, xA, zA, xB, zB, xC, zC);
}

/* adds Q=(x2:z2) and R=(x1:z1) and puts the result in (x3:z3),
using 5/6 mul, 6 add/sub and 6 mod. One assumes that Q-R=P or R-Q=P where P=(x:z).
Uses the following global variables:
- n : number to factor
- x, z : coordinates of P
- u, v, w : auxiliary variables
Modifies: x3, z3, u, v, w.
(x3,z3) may be identical to (x2,z2) and to (x,z)
*/
static void add3(limb *x3, limb *z3, limb *x2, limb *z2,
                 limb *x1, limb *z1, limb *x, limb *z)
{
  limb *t = fieldTX;
  limb *u = fieldTZ;
  limb *v = fieldUX;
  limb *w = fieldUZ;
  SubtBigNbrModN(x2, z2, v, TestNbr, NumberLength); // v = x2-z2
  AddBigNbrModN(x1, z1, w, TestNbr, NumberLength);      // w = x1+z1
  modmult(v, w, u);       // u = (x2-z2)*(x1+z1)
  AddBigNbrModN(x2, z2, w, TestNbr, NumberLength);      // w = x2+z2
  SubtBigNbrModN(x1, z1, t, TestNbr, NumberLength); // t = x1-z1
  modmult(t, w, v);       // v = (x2+z2)*(x1-z1)
  AddBigNbrModN(u, v, t, TestNbr, NumberLength);        // t = 2*(x1*x2-z1*z2)
  modmult(t, t, w);       // w = 4*(x1*x2-z1*z2)^2
  SubtBigNbrModN(u, v, t, TestNbr, NumberLength);   // t = 2*(x2*z1-x1*z2)
  modmult(t, t, v);       // v = 4*(x2*z1-x1*z2)^2
  if (!memcmp(x, x3, NumberLength*sizeof(limb)))
  {
    memcpy(u, x, NumberLength * sizeof(int));
    memcpy(t, w, NumberLength * sizeof(int));
    modmult(z, t, w);
    modmult(v, u, z3);
    memcpy(x3, w, NumberLength * sizeof(int));
  }
  else
  {
    modmult(w, z, x3); // x3 = 4*z*(x1*x2-z1*z2)^2
    modmult(x, v, z3); // z3 = 4*x*(x2*z1-x1*z2)^2
  }
}

/* computes 2P=(x2:z2) from P=(x1:z1), with 5 mul, 4 add/sub, 5 mod.
Uses the following global variables:
- n : number to factor
- b : (a+2)/4 mod n
- u, v, w : auxiliary variables
Modifies: x2, z2, u, v, w
*/
static void duplicate(limb *x2, limb *z2, limb *x1, limb *z1)
{
  limb *u = fieldUZ;
  limb *v = fieldTX;
  limb *w = fieldTZ;
  AddBigNbrModN(x1, z1, w, TestNbr, NumberLength);      // w = x1+z1
  modmult(w, w, u);       // u = (x1+z1)^2
  SubtBigNbrModN(x1, z1, w, TestNbr, NumberLength); // w = x1-z1
  modmult(w, w, v);       // v = (x1-z1)^2
  modmult(u, v, x2);      // x2 = u*v = (x1^2 - z1^2)^2
  SubtBigNbrModN(u, v, w, TestNbr, NumberLength);   // w = u-v = 4*x1*z1
  modmult(fieldAA, w, u);
  AddBigNbrModN(u, v, u, TestNbr, NumberLength);        // u = (v+b*w)
  modmult(w, u, z2);      // z2 = (w*u)
}
/* End of code adapted from Paul Zimmermann's ECM4C */

static int gcdIsOne(limb *value)
{
  UncompressLimbsBigInteger(value, &Temp1);
  UncompressLimbsBigInteger(TestNbr, &Temp2);
  BigIntGcd(&Temp1, &Temp2, &Temp3);
  CompressLimbsBigInteger(GD, &Temp3);
  if (Temp3.nbrLimbs == 1 && Temp3.limbs[0].x < 2)
  {
    return Temp3.limbs[0].x;    // GCD is less than 2.
  }
  return 2;      // GCD is greater than one.
}

static void GenerateSieve(int initial)
{
  int i, j, Q, initModQ;
  for (i = 0; i < 10*SIEVE_SIZE; i += SIEVE_SIZE)
  {
    memcpy(&sieve[i], sieve2310, SIEVE_SIZE);
  }
  j = 5;
  Q = 13; /* Point to prime 13 */
  do
  {
    if (initial > Q * Q)
    {
      initModQ = initial % Q;
      if (initModQ & 1)
      {    // initModQ is odd
        i = (Q - initModQ) >> 1;
      }
      else if (initModQ == 0)
      {
        i = 0;
      }
      else
      {    // initModQ is even
        i = Q - (initModQ >> 1);
      }
      for (; i < 10*SIEVE_SIZE; i += Q)
      {
        sieve[i] = 1; /* Composite */
      }
    }
    else
    {
      i = Q * Q - initial;
      if (i < 20*SIEVE_SIZE)
      {
        for (i = i / 2; i < 10*SIEVE_SIZE; i += Q)
        {
          sieve[i] = 1; /* Composite */
        }
      }
      else
      {
        break;
      }
    }
    Q = SmallPrime[++j];
  } while (Q < 5000);
}

static enum eEcmResult ecmCurve(void)
{
#ifdef __EMSCRIPTEN__
  char text[20];
#endif
  EC %= 50000000;   // Convert to curve number.
  for (;;)
  {
#ifdef __EMSCRIPTEN__
    char *ptrText;
#endif
    int I, Pass;
    int i, j, u;
    long long L1, L2, LS, P, IP, Paux = 1;
    if (NextEC > 0)
    {
      EC = NextEC;
      NextEC = -1;
      if (EC >= TYP_SIQS)
      {
        GD[0].x = 1;   // Set GD to 1.
        memset(&GD[1], 0, (NumberLength - 1) * sizeof(limb));
        return FACTOR_FOUND;
      }
    }
    else
    {
      EC++;
#ifdef __EMSCRIPTEN__
      text[0] = '7';
      ptrText = &text[1];
      int2dec(&ptrText, EC);
      databack(text);
#endif
#if 0
      NN = Lehman(NumberToFactor, EC);
      if (!NN.equals(BigInt1))
      {                // Factor found.
        foundByLehman = true;
        return NN;
      }
#endif
      L1 = NumberLength*9/2;        // Get number of digits.
      if (L1 > 30 && L1 <= 90 /*&&    // If between 30 and 90 digits...
        (digitsInGroup & 0x400) == 0*/)
      {                             // Switch to SIQS checkbox is set.
        int limit = limits[((int)L1 - 31) / 5];
        if (EC % 50000000 >= limit /*&& !forcedECM*/)
        {                           // Switch to SIQS.
          EC += TYP_SIQS;
          return CHANGE_TO_SIQS;
        }
      }
    }
    Typ[FactorIndex] = EC;
    L1 = 2000;
    L2 = 200000;
    LS = 45;
    Paux = EC;
    nbrPrimes = 303; /* Number of primes less than 2000 */
    if (EC > 25)
    {
      if (EC < 326)
      {
        L1 = 50000;
        L2 = 5000000;
        LS = 224;
        Paux = EC - 24;
        nbrPrimes = 5133; /* Number of primes less than 50000 */
      }
      else
      {
        if (EC < 2000)
        {
          L1 = 1000000;
          L2 = 100000000;
          LS = 1001;
          Paux = EC - 299;
          nbrPrimes = 78498; /* Number of primes less than 1000000 */
        }
        else
        {
          L1 = 11000000;
          L2 = 1100000000;
          LS = 3316;
          Paux = EC - 1900;
          nbrPrimes = 726517; /* Number of primes less than 11000000 */
        }
      }
    }
#ifdef __EMSCRIPTEN__
    ptrText = ptrLowerText;  // Point after number that is being factored.
    strcpy(ptrText, lang ? "<p>Curva ": "<p>Curve ");
    ptrText += strlen(ptrText);
    int2dec(&ptrText, EC);   // Show curve number.
    strcpy(ptrText, lang?" usando límites B1=": " using bounds B1=");
    ptrText += strlen(ptrText);
    int2dec(&ptrText, L1);   // Show first bound.
    strcpy(ptrText, lang? " y B2=" : " and B2=");
    ptrText += strlen(ptrText);
    int2dec(&ptrText, L2);   // Show second bound.
    strcpy(ptrText, "</p>");
    ptrText += strlen(ptrText);
    databack(lowerText);
#if 0
    primalityString =
      textAreaContents
      + StringToLabel
      + "\nLimit (B1="
      + L1
      + "; B2="
      + L2
      + ")    Curve ";
    UpperLine = "Digits in factor:   ";
    LowerLine = "Probability:        ";
    for (I = 0; I < 6; I++)
    {
      UpperLine += "    >= " + (I * 5 + 15);
      Prob =
        (int)Math.round(
          100
          * (1 - exp(-((double)L1 * (double)Paux) / ProbArray[I])));
      if (Prob == 100)
      {
        LowerLine += "    100% ";
      }
      else
      {
        if (Prob >= 10)
        {
          LowerLine += "     " + Prob + "% ";
        }
        else
        {
          LowerLine += "      " + Prob + "% ";
        }
      }
    } /* end for */
    lowerTextArea.setText(
      primalityString + EC + "\n" + UpperLine + "\n" + LowerLine);
#endif
#endif

    //  Compute A0 <- 2 * (EC+1)*modinv(3 * (EC+1) ^ 2 - 1, N) mod N
    NbrToLimbs(EC + 1, Aux2, NumberLength);    // Aux2 <- EC + 1
    modmult(Aux2, MontgomeryMultR2, Aux2);     // Convert Aux2 to Montgomery notation.
    modmultInt(Aux2, 2, Aux1);                 // Aux1 <- 2*(EC+1)
    modmult(Aux2, Aux2, Aux3);                 // Aux3 <- (EC + 1)^2
    modmultInt(Aux3, 3, Aux3);                 // Aux3 <- 3*(EC + 1)^2
    SubtBigNbrModN(Aux3, MontgomeryMultR1, Aux2, TestNbr, NumberLength); // Aux2 <- 3*(EC + 1)^2 - 1
    ModInvBigNbr(Aux2, Aux2, TestNbr, NumberLength);
    modmult(Aux1, Aux2, A0);

    //  if A0*(A0 ^ 2 - 1)*(9 * A0 ^ 2 - 1) mod N=0 then select another curve.
    modmult(A0, A0, A02);          // A02 <- A0^2
    modmult(A02, A0, A03);         // A03 <- A0^3
    SubtBigNbrModN(A03, A0, Aux1, TestNbr, NumberLength);  // Aux1 <- A0^3 - A0
    modmultInt(A02, 9, Aux2);      // Aux2 <- 9*A0^2
    SubtBigNbrModN(Aux2, MontgomeryMultR1, Aux2, TestNbr, NumberLength); // Aux2 <- 9*A0^2-1
    modmult(Aux1, Aux2, Aux3);
    if (BigNbrIsZero(Aux3))
    {
      continue;
    }
    //   Z <- 4 * A0 mod N
    modmultInt(A0, 4, Z);
    //   A = (-3 * A0 ^ 4 - 6 * A0 ^ 2 + 1)*modinv(4 * A0 ^ 3, N) mod N
    modmultInt(A02, 6, Aux1);      // Aux1 <- 6*A0^2
    SubtBigNbrModN(MontgomeryMultR1, Aux1, Aux1, TestNbr, NumberLength);
    modmult(A02, A02, Aux2);       // Aux2 <- A0^4
    modmultInt(Aux2, 3, Aux2);     // Aux2 <- 3*A0^4
    SubtBigNbrModN(Aux1, Aux2, Aux1, TestNbr, NumberLength);
    modmultInt(A03, 4, Aux2);      // Aux2 <- 4*A0^3
    ModInvBigNbr(Aux2, Aux3, TestNbr, NumberLength);
    modmult(Aux1, Aux3, A0);
    //   AA <- (A + 2)*modinv(4, N) mod N
    modmultInt(MontgomeryMultR1, 2, Aux2);  // Aux2 <- 2
    AddBigNbrModN(A0, Aux2, Aux1, TestNbr, NumberLength); // Aux1 <- A0+2
    modmultInt(MontgomeryMultR1, 4, Aux2);  // Aux2 <- 4
    ModInvBigNbr(Aux2, Aux2, TestNbr, NumberLength);
    modmult(Aux1, Aux2, AA);
    //   X <- (3 * A0 ^ 2 + 1) mod N
    modmultInt(A02, 3, Aux1);    // Aux1 <- 3*A0^2
    AddBigNbrModN(Aux1, MontgomeryMultR1, X, TestNbr, NumberLength);
    /**************/
    /* First step */
    /**************/
    memcpy(Xaux, X, NumberLength * sizeof(limb));
    memcpy(Zaux, Z, NumberLength * sizeof(limb));
    memcpy(GcdAccumulated, MontgomeryMultR1, (NumberLength+1) * sizeof(limb));
    for (Pass = 0; Pass < 2; Pass++)
    {
      /* For powers of 2 */
      indexPrimes = 0;
      StepECM = 1;
      for (I = 1; I <= L1; I <<= 1)
      {
        duplicate(X, Z, X, Z);
      }
      for (I = 3; I <= L1; I *= 3)
      {
        duplicate(W1, W2, X, Z);
        add3(X, Z, X, Z, W1, W2, X, Z);
      }

      if (Pass == 0)
      {
        modmult(GcdAccumulated, Z, Aux1);
        memcpy(GcdAccumulated, Aux1, NumberLength * sizeof(limb));
      }
      else
      {
        if (gcdIsOne(Z) > 1)
        {
          return FACTOR_FOUND;
        }
      }

      /* for powers of odd primes */

      indexM = 1;
      do
      {
        indexPrimes++;
        P = SmallPrime[indexM];
        for (IP = P; IP <= L1; IP *= P)
        {
          prac((int)P, X, Z, W1, W2, W3, W4);
        }
        indexM++;
        if (Pass == 0)
        {
          modmult(GcdAccumulated, Z, Aux1);
          memcpy(GcdAccumulated, Aux1, NumberLength * sizeof(limb));
        }
        else
        {
          if (gcdIsOne(Z) > 1)
          {
            return FACTOR_FOUND;
          }
        }
      } while (SmallPrime[indexM - 1] <= LS);
      P += 2;

      /* Initialize sieve2310[n]: 1 if gcd(P+2n,2310) > 1, 0 otherwise */
      u = (int)P;
      for (i = 0; i < SIEVE_SIZE; i++)
      {
        sieve2310[i] =
          (u % 3 == 0
            || u % 5 == 0
            || u % 7 == 0? (unsigned char)1 : (unsigned char)0);
        u += 2;
      }
      do
      {
        /* Generate sieve */
        GenerateSieve((int)P);

        /* Walk through sieve */

        for (i = 0; i < 10*SIEVE_SIZE; i++)
        {
          if (sieve[i] != 0)
            continue; /* Do not process composites */
          if (P + 2 * i > L1)
            break;
          indexPrimes++;
          prac((int)(P + 2 * i), X, Z, W1, W2, W3, W4);
          if (Pass == 0)
          {
            modmult(GcdAccumulated, Z, Aux1);
            memcpy(GcdAccumulated, Aux1, NumberLength * sizeof(limb));
          }
          else
          {
            if (gcdIsOne(Z) > 1)
            {
              return FACTOR_FOUND;
            }
          }
        }
        P += 20*SIEVE_SIZE;
      } while (P < L1);
      if (Pass == 0)
      {
        if (BigNbrIsZero(GcdAccumulated))
        { // If GcdAccumulated is
          memcpy(X, Xaux, NumberLength * sizeof(limb));
          memcpy(Z, Zaux, NumberLength * sizeof(limb));
          continue; // multiple of TestNbr, continue.
        }
        if (gcdIsOne(GcdAccumulated) > 1)
        {
          return 1;
        }
        break;
      }
    } /* end for Pass */

      /******************************************************/
      /* Second step (using improved standard continuation) */
      /******************************************************/
    StepECM = 2;
    j = 0;
    for (u = 1; u < SIEVE_SIZE; u += 2)
    {
      if (u % 3 == 0 || u % 5 == 0 || u % 7 == 0)
      {
        sieve2310[u / 2] = (unsigned char)1;
      }
      else
      {
        sieve2310[(sieveidx[j++] = u / 2)] = (unsigned char)0;
      }
    }
    memcpy(&sieve2310[HALF_SIEVE_SIZE], &sieve2310[0], HALF_SIEVE_SIZE);
    memcpy(Xaux, X, NumberLength * sizeof(limb));  // (X:Z) -> Q (output
    memcpy(Zaux, Z, NumberLength * sizeof(limb));  //         from step 1)
    for (Pass = 0; Pass < 2; Pass++)
    {
      int Qaux, J;
      memcpy(GcdAccumulated, MontgomeryMultR1, NumberLength * sizeof(limb));
      memcpy(UX, X, NumberLength * sizeof(limb));
      memcpy(UZ, Z, NumberLength * sizeof(limb));  // (UX:UZ) -> Q 
      ModInvBigNbr(Z, Aux1, TestNbr, NumberLength);
      modmult(Aux1, X, root[0]); // root[0] <- X/Z (Q)
      J = 0;
      AddBigNbrModN(X, Z, Aux1, TestNbr, NumberLength);
      modmult(Aux1, Aux1, W1);
      SubtBigNbrModN(X, Z, Aux1, TestNbr, NumberLength);
      modmult(Aux1, Aux1, W2);
      modmult(W1, W2, TX);
      SubtBigNbrModN(W1, W2, Aux1, TestNbr, NumberLength);
      modmult(Aux1, AA, Aux2);
      AddBigNbrModN(Aux2, W2, Aux3, TestNbr, NumberLength);
      modmult(Aux1, Aux3, TZ); // (TX:TZ) -> 2Q
      SubtBigNbrModN(X, Z, Aux1, TestNbr, NumberLength);
      AddBigNbrModN(TX, TZ, Aux2, TestNbr, NumberLength);
      modmult(Aux1, Aux2, W1);
      AddBigNbrModN(X, Z, Aux1, TestNbr, NumberLength);
      SubtBigNbrModN(TX, TZ, Aux2, TestNbr, NumberLength);
      modmult(Aux1, Aux2, W2);
      AddBigNbrModN(W1, W2, Aux1, TestNbr, NumberLength);
      modmult(Aux1, Aux1, Aux2);
      modmult(Aux2, UZ, X);
      SubtBigNbrModN(W1, W2, Aux1, TestNbr, NumberLength);
      modmult(Aux1, Aux1, Aux2);
      modmult(Aux2, UX, Z); // (X:Z) -> 3Q
      for (I = 5; I < SIEVE_SIZE; I += 2)
      {
        memcpy(WX, X, NumberLength * sizeof(limb));
        memcpy(WZ, Z, NumberLength * sizeof(limb));
        SubtBigNbrModN(X, Z, Aux1, TestNbr, NumberLength);
        AddBigNbrModN(TX, TZ, Aux2, TestNbr, NumberLength);
        modmult(Aux1, Aux2, W1);
        AddBigNbrModN(X, Z, Aux1, TestNbr, NumberLength);
        SubtBigNbrModN(TX, TZ, Aux2, TestNbr, NumberLength);
        modmult(Aux1, Aux2, W2);
        AddBigNbrModN(W1, W2, Aux1, TestNbr, NumberLength);
        modmult(Aux1, Aux1, Aux2);
        modmult(Aux2, UZ, X);
        SubtBigNbrModN(W1, W2, Aux1, TestNbr, NumberLength);
        modmult(Aux1, Aux1, Aux2);
        modmult(Aux2, UX, Z); // (X:Z) -> 5Q, 7Q, ...
        if (Pass == 0)
        {
          modmult(GcdAccumulated, Aux1, Aux2);
          memcpy(GcdAccumulated, Aux2, NumberLength * sizeof(limb));
        }
        else
        {
          if (gcdIsOne(Aux1) > 1)
          {
            return FACTOR_FOUND;
          }
        }
        if (I == HALF_SIEVE_SIZE)
        {
          memcpy(DX, X, NumberLength * sizeof(limb));
          memcpy(DZ, Z, NumberLength * sizeof(limb));  // (DX:DZ) -> HALF_SIEVE_SIZE*Q
        }
        if (I % 3 != 0 && I % 5 != 0 && I % 7 != 0)
        {
          J++;
          ModInvBigNbr(Z, Aux1, TestNbr, NumberLength);
          modmult(Aux1, X, root[J]); // root[J] <- X/Z
        }
        memcpy(UX, WX, NumberLength * sizeof(limb));  // (UX:UZ) <-
        memcpy(UZ, WZ, NumberLength * sizeof(limb));  // Previous (X:Z)
      } /* end for I */
      AddBigNbrModN(DX, DZ, Aux1, TestNbr, NumberLength);
      modmult(Aux1, Aux1, W1);
      SubtBigNbrModN(DX, DZ, Aux1, TestNbr, NumberLength);
      modmult(Aux1, Aux1, W2);
      modmult(W1, W2, X);
      SubtBigNbrModN(W1, W2, Aux1, TestNbr, NumberLength);
      modmult(Aux1, AA, Aux2);
      AddBigNbrModN(Aux2, W2, Aux3, TestNbr, NumberLength);
      modmult(Aux1, Aux3, Z);
      memcpy(UX, X, NumberLength * sizeof(limb));
      memcpy(UZ, Z, NumberLength * sizeof(limb));    // (UX:UZ) -> SIEVE_SIZE*Q
      AddBigNbrModN(X, Z, Aux1, TestNbr, NumberLength);
      modmult(Aux1, Aux1, W1);
      SubtBigNbrModN(X, Z, Aux1, TestNbr, NumberLength);
      modmult(Aux1, Aux1, W2);
      modmult(W1, W2, TX);
      SubtBigNbrModN(W1, W2, Aux1, TestNbr, NumberLength);
      modmult(Aux1, AA, Aux2);
      AddBigNbrModN(Aux2, W2, Aux3, TestNbr, NumberLength);
      modmult(Aux1, Aux3, TZ); // (TX:TZ) -> 2*SIEVE_SIZE*Q
      SubtBigNbrModN(X, Z, Aux1, TestNbr, NumberLength);
      AddBigNbrModN(TX, TZ, Aux2, TestNbr, NumberLength);
      modmult(Aux1, Aux2, W1);
      AddBigNbrModN(X, Z, Aux1, TestNbr, NumberLength);
      SubtBigNbrModN(TX, TZ, Aux2, TestNbr, NumberLength);
      modmult(Aux1, Aux2, W2);
      AddBigNbrModN(W1, W2, Aux1, TestNbr, NumberLength);
      modmult(Aux1, Aux1, Aux2);
      modmult(Aux2, UZ, X);
      SubtBigNbrModN(W1, W2, Aux1, TestNbr, NumberLength);
      modmult(Aux1, Aux1, Aux2);
      modmult(Aux2, UX, Z); // (X:Z) -> 3*SIEVE_SIZE*Q
      Qaux = (int)(L1 / (2*SIEVE_SIZE));
      maxIndexM = (int)(L2 / (2 * SIEVE_SIZE));
      for (indexM = 0; indexM <= maxIndexM; indexM++)
      {
        if (indexM >= Qaux)
        { // If inside step 2 range... 
          if (indexM == 0)
          {
            ModInvBigNbr(UZ, Aux3, TestNbr, NumberLength);
            modmult(UX, Aux3, Aux1); // Aux1 <- X/Z (SIEVE_SIZE*Q)
          }
          else
          {
            ModInvBigNbr(Z, Aux3, TestNbr, NumberLength);
            modmult(X, Aux3, Aux1); // Aux1 <- X/Z (3,5,*
          }                         //         SIEVE_SIZE*Q)

            /* Generate sieve */
          if (indexM % 10 == 0 || indexM == Qaux)
          {
            GenerateSieve(indexM / 10 * (20*SIEVE_SIZE) + 1);
          }
          /* Walk through sieve */
          J = HALF_SIEVE_SIZE + (indexM % 10) * SIEVE_SIZE;
          for (i = 0; i < GROUP_SIZE; i++)
          {
            j = sieveidx[i]; // 0 < J < HALF_SIEVE_SIZE
            if (sieve[J + j] != 0 && sieve[J - 1 - j] != 0)
            {
              continue; // Do not process if both are composite numbers.
            }
            SubtBigNbrModN(Aux1, root[i], M, TestNbr, NumberLength);
            modmult(GcdAccumulated, M, Aux2);
            memcpy(GcdAccumulated, Aux2, NumberLength * sizeof(limb));
          }
          if (Pass != 0)
          {
            if (gcdIsOne(GcdAccumulated) > 1)
            {
              return FACTOR_FOUND;
            }
          }
        }
        if (indexM != 0)
        { // Update (X:Z)
          memcpy(WX, X, NumberLength * sizeof(limb));
          memcpy(WZ, Z, NumberLength * sizeof(limb));
          SubtBigNbrModN(X, Z, Aux1, TestNbr, NumberLength);
          AddBigNbrModN(TX, TZ, Aux2, TestNbr, NumberLength);
          modmult(Aux1, Aux2, W1);
          AddBigNbrModN(X, Z, Aux1, TestNbr, NumberLength);
          SubtBigNbrModN(TX, TZ, Aux2, TestNbr, NumberLength);
          modmult(Aux1, Aux2, W2);
          AddBigNbrModN(W1, W2, Aux1, TestNbr, NumberLength);
          modmult(Aux1, Aux1, Aux2);
          modmult(Aux2, UZ, X);
          SubtBigNbrModN(W1, W2, Aux1, TestNbr, NumberLength);
          modmult(Aux1, Aux1, Aux2);
          modmult(Aux2, UX, Z);
          memcpy(UX, WX, NumberLength * sizeof(limb));
          memcpy(UZ, WZ, NumberLength * sizeof(limb));
        }
      } // end for Q
      if (Pass == 0)
      {
        int rc;
        if (BigNbrIsZero(GcdAccumulated))
        { // If GcdAccumulated is zero
          memcpy(X, Xaux, NumberLength * sizeof(limb));
          memcpy(Z, Zaux, NumberLength * sizeof(limb));
          continue; // multiple of TestNbr, continue.
        }
        rc = gcdIsOne(GcdAccumulated);
        if (rc == 1)
        {
          break;    // GCD is one, so this curve does not find a factor.
        }
        if (rc == 0)
        {
          continue;
        }
                    // GD <- GCD(GcdAccumulated, TestNbr)
        if (memcmp(GD, TestNbr, NumberLength*sizeof(limb)))
        {           // GCD is not 1 or TestNbr
          return FACTOR_FOUND;
        }
      }
    } /* end for Pass */
    performLehman = TRUE;
  }       /* End curve calculation */
}

static void ecm(BigInteger *N, struct sFactors *pstFactors)
{
  int P, Q;
  fieldTX = TX;
  fieldTZ = TZ;
  fieldUX = UX;
  fieldUZ = UZ;
  int Prob;
  BigInteger NN;

  fieldAA = AA;
#ifdef __EMSCRIPTEN__
  GetYieldFrequency();
#endif
  GetMontgomeryParms(NumberLength);
  memset(M, 0, NumberLength * sizeof(limb));
  memset(DX, 0, NumberLength * sizeof(limb));
  memset(DZ, 0, NumberLength * sizeof(limb));
  memset(W3, 0, NumberLength * sizeof(limb));
  memset(W4, 0, NumberLength * sizeof(limb));
  memset(GD, 0, NumberLength * sizeof(limb));
#ifdef __EMSCRIPTEN__
  ptrLowerText = lowerText;
  *ptrLowerText++ = '3';
  if (pstFactors->multiplicity > 1)
  {    // Some factorization known.
    strcpy(ptrLowerText, "<p class=\"blue\">");
    ptrLowerText += strlen(ptrLowerText);
    SendFactorizationToOutput(EXPR_OK, pstFactors, &ptrLowerText, 1);
    strcpy(ptrLowerText, "</p>");
    ptrLowerText += strlen(ptrLowerText);
  }
  strcpy(ptrLowerText, lang ? "<p>Factorizando ": "<p>Factoring " );
  ptrLowerText += strlen(ptrLowerText);
  Bin2Dec(N->limbs, ptrLowerText, N->nbrLimbs, groupLen);
  ptrLowerText += strlen(ptrLowerText);
  strcpy(ptrLowerText, "</p>");
  ptrLowerText += strlen(ptrLowerText);
#endif
  EC--;
  SmallPrime[0] = 2;
  P = 3;
  indexM = 1;
  for (indexM = 1; indexM < sizeof(SmallPrime)/sizeof(SmallPrime[0]); indexM++)
  {     // Loop that fills the SmallPrime array.
    SmallPrime[indexM] = (int)P; /* Store prime */
    do
    {
      P += 2;
      for (Q = 3; Q * Q <= P; Q += 2)
      { /* Check if P is prime */
        if (P % Q == 0)
        {
          break;  /* Composite */
        }
      }
    } while (Q * Q <= P);
  }
  foundByLehman = FALSE;
  do
  {
    enum eEcmResult ecmResp = ecmCurve();
    if (ecmResp == CHANGE_TO_SIQS)
    {    // Perform SIQS
      FactoringSIQS(TestNbr, GD);
      break;
    }
    else if (ecmResp == FACTOR_FOUND)
    {
      break;
    }
  } while (!memcmp(GD, TestNbr, NumberLength*sizeof(limb)));
#if 0
  lowerTextArea.setText("");
#endif
  StepECM = 0; /* do not show pass number on screen */
  return;
}

void SendFactorizationToOutput(enum eExprErr rc, struct sFactors *pstFactors, char **pptrOutput, int doFactorization)
{
  char *ptrOutput = *pptrOutput;
  if (rc != EXPR_OK)
  {
    textError(ptrOutput, rc);
    ptrOutput += strlen(ptrOutput);
  }
  else
  {
    Bin2Dec(tofactor.limbs, ptrOutput, tofactor.nbrLimbs, groupLen);
    ptrOutput += strlen(ptrOutput);
    if (doFactorization)
    {
      struct sFactors *pstFactor;
      int i = 0;
      pstFactor = pstFactors+1;
      if (pstFactors->multiplicity == 1 && pstFactor->multiplicity == 1 &&
        (*pstFactor->ptrFactor > 1 || *(pstFactor->ptrFactor + 1) > 1))
      {    // Do not show zero or one as prime.
        strcpy(ptrOutput, lang ? " es primo" : " is prime");
        ptrOutput += strlen(ptrOutput);
      }
      else
      {
        strcpy(ptrOutput, " = ");
        ptrOutput += strlen(ptrOutput);
        for (;;)
        {
          UncompressBigInteger(pstFactor->ptrFactor, &factorValue);
          Bin2Dec(factorValue.limbs, ptrOutput, factorValue.nbrLimbs, groupLen);
          ptrOutput += strlen(ptrOutput);
          if (pstFactor->multiplicity > 1)
          {
            strcpy(ptrOutput, "<sup>");
            ptrOutput += strlen(ptrOutput);
            int2dec(&ptrOutput, pstFactor->multiplicity);
            strcpy(ptrOutput, "</sup>");
            ptrOutput += strlen(ptrOutput);
          }
          if (++i == pstFactors->multiplicity)
          {
            break;
          }
          strcpy(ptrOutput, " &times; ");
          ptrOutput += strlen(ptrOutput);
          pstFactor++;
        }
      }
    }
  }
  *pptrOutput = ptrOutput;
}

static void SortFactors(struct sFactors *pstFactors)
{
  int factorNumber, factorNumber2, ctr;
  struct sFactors *pstCurFactor = pstFactors + 1;
  struct sFactors stTempFactor;
  int *ptrNewFactor;
  struct sFactors *pstNewFactor;
  for (factorNumber = 1; factorNumber <= pstFactors->multiplicity; factorNumber++, pstCurFactor++)
  {
    int *ptrFactor, *ptrFactor2;
    pstNewFactor = pstCurFactor + 1;
    for (factorNumber2 = factorNumber + 1; factorNumber2 <= pstFactors->multiplicity; factorNumber2++, pstNewFactor++)
    {
      ptrFactor = pstCurFactor->ptrFactor;
      ptrFactor2 = pstNewFactor->ptrFactor;
      if (*ptrFactor < *ptrFactor2)
      {     // Factors already in correct order.
        continue;
      }
      if (*ptrFactor == *ptrFactor2)
      {
        for (ctr = *ptrFactor; ctr > 1; ctr--)
        {
          if (*(ptrFactor + ctr) != *(ptrFactor2 + ctr))
          {
            break;
          }
        }
        if (*(ptrFactor + ctr) < *(ptrFactor2 + ctr))
        {     // Factors already in correct order.
          continue;
        }
        if (*(ptrFactor + ctr) == *(ptrFactor2 + ctr))
        {     // Factors are the same.
          pstCurFactor->multiplicity += pstNewFactor->multiplicity;
          ctr = pstFactors->multiplicity - factorNumber2;
          if (ctr > 0)
          {
            memmove(pstNewFactor, pstNewFactor + 1, ctr * sizeof(struct sFactors));
          }
          pstFactors->multiplicity--;   // Indicate one less known factor.
          continue;
        }
      }
      // Exchange both factors.
      memcpy(&stTempFactor, pstCurFactor, sizeof(struct sFactors));
      memcpy(pstCurFactor, pstNewFactor, sizeof(struct sFactors));
      memcpy(pstNewFactor, &stTempFactor, sizeof(struct sFactors));
    }
  }
  // Find location for new factors.
  ptrNewFactor = 0;
  pstCurFactor = pstFactors + 1;
  for (factorNumber = 1; factorNumber <= pstFactors->multiplicity; factorNumber++, pstCurFactor++)
  {
    int *ptrPotentialNewFactor = pstCurFactor->ptrFactor + *(pstCurFactor->ptrFactor) + 1;
    if (ptrPotentialNewFactor > ptrNewFactor)
    {
      ptrNewFactor = ptrPotentialNewFactor;
    }
  }
  pstFactors->ptrFactor = ptrNewFactor;
}

// Insert new factor found into factor array. This factor array must be sorted.
static void insertIntFactor(struct sFactors *pstFactors, struct sFactors *pstFactorDividend, int divisor)
{
  struct sFactors *pstCurFactor;
  int ctr, factorNumber;
  int remainder = 0;
  int *ptrFactor = pstFactorDividend->ptrFactor;
  int nbrLimbs = *ptrFactor;
  int *ptrValue = ptrFactor + nbrLimbs;
  pstFactorDividend->upperBound = divisor;

  // Divide number by factor just found.
  for (ctr = nbrLimbs - 1; ctr >= 0; ctr--)
  {
    remainder = (remainder << BITS_PER_GROUP) + *ptrValue;
    *ptrValue-- = remainder / divisor;
    remainder %= divisor;
  }
  if (*(ptrFactor + nbrLimbs) == 0)
  {
    (*ptrFactor)--;
  }
  // Check whether prime is already in factor list.
  pstCurFactor = pstFactors+1;
  for (factorNumber = 1; factorNumber <= pstFactors->multiplicity; factorNumber++, pstCurFactor++)
  {
    ptrValue = pstCurFactor->ptrFactor;  // Point to factor in factor array.
    if (*ptrValue == 1 && *(ptrValue+1) == divisor)
    {  // Prime already found: increment multiplicity and go out.
      pstCurFactor->multiplicity += pstFactorDividend->multiplicity;
      ptrValue = pstFactorDividend->ptrFactor;
      if (*ptrValue == 1 && *(ptrValue + 1) == 1)
      {    // Dividend is 1 now so discard it.
        *pstFactorDividend = *(pstFactors + pstFactors->multiplicity--);
      }
      SortFactors(pstFactors);
      return;
    }
    if (*ptrValue > 1 || *(ptrValue + 1) > divisor)
    {   // Factor in factor list is greater than factor to insert. Exit loop.
      break;
    }
  }
  pstFactors->multiplicity++; // Indicate new known factor.
  // Move all elements.
  ptrValue = pstFactorDividend->ptrFactor;
  if (pstFactors->multiplicity > factorNumber)
  {
    memmove(pstCurFactor + 1, pstCurFactor,
      (pstFactors->multiplicity - factorNumber) * sizeof(struct sFactors));
  }
  if (*ptrValue == 1 && *(ptrValue + 1) == 1)
  {
    pstCurFactor = pstFactorDividend;
  }
  else
  {
    ptrValue = pstFactors->ptrFactor;
    pstCurFactor->ptrFactor = ptrValue;
    pstCurFactor->multiplicity = pstFactorDividend->multiplicity;
    pstFactors->ptrFactor += 2;  // Next free memory.
  }
  pstCurFactor->upperBound = 0;
  *ptrValue = 1;  // Number of limbs.
  *(ptrValue + 1) = divisor;
  SortFactors(pstFactors);
}

// Insert new factor found into factor array. This factor array must be sorted.
// The divisor must be also sorted.
static void insertBigFactor(struct sFactors *pstFactors, BigInteger *divisor)
{
  struct sFactors *pstCurFactor;
  int factorNumber;
  int lastFactorNumber = pstFactors->multiplicity;
  struct sFactors *pstNewFactor = pstFactors + lastFactorNumber + 1;

  pstCurFactor = pstFactors + 1;
  for (factorNumber = 1; factorNumber <= lastFactorNumber; factorNumber++, pstCurFactor++)
  {     // For each known factor...
    int *ptrFactor = pstCurFactor->ptrFactor;
    UncompressBigInteger(ptrFactor, &Temp2);    // Convert known factor to Big Integer.
    BigIntGcd(divisor, &Temp2, &Temp3);          // Temp3 is the GCD between known factor and divisor.
    if (Temp3.nbrLimbs == 1 && Temp3.limbs[0].x < 2)
    {                                           // divisor is not a new factor (GCD = 0 or 1).
      continue;
    }
    if (TestBigNbrEqual(&Temp2, &Temp3))
    {                                           // GCD is equal to known factor.
      continue;
    }
    // At this moment both GCD and known factor / GCD are new known factors. Replace the known factor by
    // known factor / GCD and generate a new known factor entry.
    NumberLength = Temp3.nbrLimbs;
    CompressBigInteger(pstFactors->ptrFactor, &Temp3);  // Append new known factor.
    BigIntDivide(&Temp2, divisor, &Temp3);
    NumberLength = Temp3.nbrLimbs;
    CompressBigInteger(ptrFactor, &Temp3);              // Overwrite old known factor.
    pstNewFactor->multiplicity = pstCurFactor->multiplicity;
    pstNewFactor->ptrFactor = pstFactors->ptrFactor;
    pstNewFactor->upperBound = pstCurFactor->upperBound;
    pstNewFactor++;
    pstFactors->multiplicity++;
  }
  // Sort factors in ascending order. If two factors are equal, coalesce them.
  // Divide number by factor just found.
  SortFactors(pstFactors);
}
#ifdef __EMSCRIPTEN__
static void showECMStatus(void)
{
  char status[200];
  int elapsedTime;
  char *ptrStatus;
  if ((lModularMult % yieldFreq) != 0)
  {
    return;
  }
  elapsedTime = (int)(tenths() - originalTenthSecond);
  if (elapsedTime / 10 == oldTimeElapsed / 10)
  {
    return;
  }
  oldTimeElapsed = elapsedTime;
  ptrStatus = status;
  strcpy(ptrStatus, lang ? "4<p>Transcurrió " : "4<p>Time elapsed: ");
  ptrStatus += strlen(ptrStatus);
  GetDHMS(&ptrStatus, elapsedTime / 10);
  switch (StepECM)
  {
  case 1:
    strcpy(ptrStatus, lang ? "&nbsp;&nbsp;&nbsp;Paso 1: " : "&nbsp;&nbsp;&nbsp;Step 1: ");
    ptrStatus += strlen(ptrStatus);
    int2dec(&ptrStatus, indexPrimes / (nbrPrimes / 100));
    *ptrStatus++ = '%';
    break;
  case 2:
    strcpy(ptrStatus, lang ? "&nbsp;&nbsp;&nbsp;Paso 2: " : "&nbsp;&nbsp;&nbsp;Step 2: ");
    ptrStatus += strlen(ptrStatus);
    int2dec(&ptrStatus, maxIndexM == 0 ? 0 : indexM / (maxIndexM / 100));
    *ptrStatus++ = '%';
    break;
  }
  strcpy(ptrStatus, "</p>");
  databack(status);
}

// Save factors in Web Storage so factorization can continue the next time the application runs.
static void SaveFactors(struct sFactors *pstFactors)
{
  struct sFactors *pstCurFactor = pstFactors + 1;
  int factorNbr, expon;
  char text[30000];
  char *ptrText;
  int *ptrFactor;
  BigInteger bigint;
  ptrText = text;
  *ptrText++ = '8';
  strcpy(ptrText, ptrInputText);
  ptrText += strlen(ptrText);
  *ptrText++ = '=';
  for (factorNbr = 1; factorNbr <= pstFactors->multiplicity; factorNbr++, pstCurFactor++)
  {
    if (factorNbr > 1)
    {
      *ptrText++ = '*';
    }
    UncompressBigInteger(pstCurFactor->ptrFactor, &bigint);
    bigint.sign = SIGN_POSITIVE;
    BigInteger2Dec(&bigint, ptrText, -100000);
    ptrText += strlen(ptrText);
    *ptrText++ = '^';
    int2dec(&ptrText, pstCurFactor->multiplicity);
    *ptrText++ = '(';
    int2dec(&ptrText, pstCurFactor->upperBound);
    *ptrText++ = ')';
  }
  *ptrText++ = 0;
  databack(text);
}

#endif

char *findChar(char *str, char c)
{
  while (*str != 0)
  {
    if (*str == c)
    {
      return str;
    }
    str++;
  }
  return NULL;
}

static int getNextInteger(char **ppcFactors, int *result, char delimiter)
{
  char *pcFactors = *ppcFactors;
  char *ptrCharFound = findChar(pcFactors, delimiter);
  if (ptrCharFound == NULL)
  {
    return 1;
  }
  *ptrCharFound = 0;
  Dec2Bin(pcFactors, Temp1.limbs, (int)(ptrCharFound - pcFactors), &Temp1.nbrLimbs);
  if (Temp1.nbrLimbs != 1)
  {   // Exponent is too large.
    return 1;
  }
  *result = (int)Temp1.limbs[0].x;
  *ppcFactors = ptrCharFound+1;
  return 0;
}

// pstFactors -> ptrFactor points to end of factors.
// pstFactors -> multiplicity indicates the number of different factors.
void factor(int *number, int *factors, struct sFactors *pstFactors, char *pcKnownFactors)
{
  struct sFactors *pstCurFactor;
  int factorNbr, expon;
  int remainder, nbrLimbs, ctr;
  int *ptrValue, *ptrFactor;
  int dividend, multiplicity;
  int factorUpperBound;
  int restartFactoring = FALSE;
  char *ptrCharFound;
  EC = 1;
#ifdef __EMSCRIPTEN__
  oldTimeElapsed = 0;
  originalTenthSecond = tenths();
  modmultCallback = showECMStatus;   // Set callback.
#endif
  memcpy(factors, number, (1 + *number)*sizeof(int));
  pstFactors->multiplicity = 1;
  pstFactors->ptrFactor = factors + 1 + *factors;
  pstFactors->upperBound = 0;
  pstCurFactor = pstFactors + 1;
  pstCurFactor->multiplicity = 1;
  pstCurFactor->ptrFactor = factors;
  pstCurFactor->upperBound = 2;
  if (pcKnownFactors != NULL)
  {   // Insert factors saved on Web Storage.
    while (*pcKnownFactors != 0)
    {
      int *ptrNewFactor = pstFactors->ptrFactor;
      ptrCharFound = findChar(pcKnownFactors, '^');
      if (ptrCharFound == NULL)
      {
        break;
      }
      *ptrCharFound = 0;
      Dec2Bin(pcKnownFactors, prime.limbs, (int)(ptrCharFound - pcKnownFactors), &prime.nbrLimbs);
      CompressBigInteger(ptrNewFactor, &prime);
      pcKnownFactors = ptrCharFound + 1;
      if (getNextInteger(&pcKnownFactors, &multiplicity, '('))
      {     // Error on processing exponent.
        break;
      }
      if (getNextInteger(&pcKnownFactors, &factorUpperBound, ')'))
      {     // Error on processing upper bound.
        break;
      }
      for (ctr = 0; ctr < multiplicity; ctr++)
      {
        insertBigFactor(pstFactors, &prime);
      }
      if (*pcKnownFactors == '*')
      {
        pcKnownFactors++;  // Skip multiplication sign.
      }
    }
  }
#ifdef __EMSCRIPTEN__
  SaveFactors(pstFactors);
#endif
  for (factorNbr = 1; factorNbr <= pstFactors->multiplicity; factorNbr++, pstCurFactor++)
  {
    int upperBound = pstCurFactor->upperBound;
    restartFactoring = FALSE;
    // If number is prime, do not process it.
    if (upperBound == 0)
    {     // Factor is prime.
      continue;
    }
    ptrFactor = pstCurFactor->ptrFactor;
    nbrLimbs = *ptrFactor;
    while (upperBound < LIMB_RANGE && nbrLimbs > 2)
    {        // Number has at least 3 limbs: Trial division by small numbers.
      while (pstCurFactor->upperBound != 0)
      {            // Factor found.
        remainder = 0;
        ptrFactor = pstCurFactor->ptrFactor;
        nbrLimbs = *ptrFactor;
        ptrValue = ptrFactor + nbrLimbs;
        for (ctr = nbrLimbs - 1; ctr >= 0; ctr--)
        {
          remainder = ((remainder << BITS_PER_GROUP) + *ptrValue--) % upperBound;
        }
        if (remainder != 0)
        {    // Factor not found. Use new divisor.
          break;
        }
        insertIntFactor(pstFactors, pstCurFactor, upperBound);
        restartFactoring = TRUE;
        break;
      }
      if (restartFactoring)
      {
        factorNbr = 0;
        pstCurFactor = pstFactors;
        break;
      }
      if (nbrLimbs <= 2)
      {     // Number completely factored.
        break;
      }
      if (upperBound == 2)
      {
        upperBound++;
      }
      else if (upperBound%6 == 1)
      {
        upperBound += 4;
      }
      else
      {
        upperBound += 2;
      }
    }
    if (restartFactoring)
    {
      continue;
    }
    if (nbrLimbs <= 2)
    {
      if (nbrLimbs == 1)
      {
        dividend = *(ptrFactor + 1);
      }
      else
      {
        dividend = *(ptrFactor + 2)*LIMB_RANGE + *(ptrFactor + 1);
      }
      while (upperBound*upperBound <= dividend)
      {              // Trial division by small numbers.
        if (dividend % upperBound == 0)
        {            // Factor found.
          insertIntFactor(pstFactors, pstCurFactor, upperBound);
          restartFactoring = TRUE;
          break;
        }
        if (upperBound == 2)
        {
          upperBound++;
        }
        else
        {
          upperBound += 2;
        }
      }
      if (restartFactoring)
      {
        factorNbr = 0;
        pstCurFactor = pstFactors;
        continue;
      }
      pstCurFactor->upperBound = 0;   // Number is prime.
      continue;
    }
    // No small factor. Check whether the number is prime or prime power.
    UncompressBigInteger(pstCurFactor->ptrFactor, &power);
    NumberLength = power.nbrLimbs;
    expon = PowerCheck(&power, &prime);
    if (expon > 1)
    {
      CompressBigInteger(pstCurFactor->ptrFactor, &prime);
      pstCurFactor->multiplicity *= expon;
    }
    if (isPseudoprime(&prime))
    {   // Number is prime power.
      pstCurFactor->upperBound = 0;   // Indicate that number is prime.
      continue;             // Check next factor.
    }
    else
    {
      ecm(&prime, pstFactors);        // Factor number.
      // Check whether GD is not one. In this case we found a proper factor.
      for (ctr = 1; ctr < NumberLength; ctr++)
      {
        if (GD[ctr].x != 0)
        {
          break;
        }
      }
      if (ctr != NumberLength || GD[0].x != 1)
      {
        int nbrLimbs;
        Temp1.sign = SIGN_POSITIVE;
        nbrLimbs = NumberLength;
        while (nbrLimbs > 1)
        {
          if (GD[nbrLimbs-1].x != 0)
          {
            break;
          }
          nbrLimbs--;
        }
        memcpy(Temp1.limbs, GD, nbrLimbs * sizeof(limb));
        Temp1.nbrLimbs = nbrLimbs;
        insertBigFactor(pstFactors, &Temp1);
#ifdef __EMSCRIPTEN__
        SaveFactors(pstFactors);
#endif
        factorNbr = 0;
        pstCurFactor = pstFactors;
        continue;
      }
    }
  }
#ifdef __EMSCRIPTEN__
  SaveFactors(pstFactors);
#endif
}

