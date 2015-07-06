function compile_mex

    mex -v mex_integration.c integration.c ...
        -I/usr/local/include/ ...
        -L/usr/local/lib/ -lgsl -lgslcblas -lm ...
        CFLAGS="\$CFLAGS -std=c99"

end