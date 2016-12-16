# Branch-Prediction
Branch prediction competition project. Part of CSE 6421 OSU.

Implementation of a slightly modified version of TAGE predictor.

Description:<br/>
    • Inspired from PPM like,TAG based and L-TAGE predictors.<br/>
    • 1 base table.<br />
        • Total number of entries 2^13 <br/>
        • Uses 2 bits saturating counter for prediction<br />
        • Hence Size = 16384 bits.<br />
    • 12 Tagged tables.
        • Total number of entries 2^11
        • Uses 3 bits counter for prediction, 15bits for tag (one table only uses 14 bits as tag), 2 bits for useful bits.
        • Hence size = 450560 + 38912 = 489472 bits
    • Global history – 641 bits.
    • Path history – 32bits.
    • Alternate prediction global counter 4bits.
    • Hence Total Sizeof the predictor = 506533 bits = 61.83 Kbytes < 62 KB

Steps to Run:
    1. Download the simulation kit from http://www.jilp.org/cbp2016/framework.html
    2. Place the predictor.cc and predictor.h files under sim director.
    3. Follow the instructions in the kit to make.
    4. Run it on the traces.

References:
    1. http://www.jilp.org/vol7/v7paper10.pdf
    2. http://www.jilp.org/vol9/v9paper6.pdf
