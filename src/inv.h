//Compute multiplicative inverses

//This code is borrowed from https://rosettacode.org/wiki/Modular_inverse#Recursive_implementation
int mul_inv(int a, int b)
{
        int t, nt, r, nr, q, tmp;
	if (b < 0) b = -b;
        if (a < 0) a = b - (-a % b);
        t = 0;  nt = 1;  r = b;  nr = a % b;
        while (nr != 0) {
          q = r/nr;
          tmp = nt;  nt = t - q*nt;  t = tmp;
          tmp = nr;  nr = r - q*nr;  r = tmp;
        }
        if (r > 1) return 0;  /* No inverse */
        if (t < 0) t += b;
        return t;
}



void computeInverses(BT *INV) {
  for (BT i = 1; i < MODULUS; i++) {
    INV[i] = mul_inv(i, MODULUS); 
  }  
}
