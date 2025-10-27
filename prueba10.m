inicio
    real suma;
    entero i;

    suma := 0.0;
    i := 1;

    mientras i <= 3
        si i != 2 entonces
            suma := suma + (i + 0.5);
        sino
            suma := suma + 1.0;
        finsi

        escribir(i);
        i := i + 1;
    finmientras

    escribir(suma);
fin