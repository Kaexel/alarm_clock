//denne funksjonen finner forskjellen mellom to tall.
//dette er nyttig for å f.eks. ikke oppdatere LCD for ofte
int difference(int a, int b) {

  if(a > b) {
    return a-b;
  } else {
    return b-a;
  }
}