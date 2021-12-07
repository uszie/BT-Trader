#include "bttrader.h"
#include "blur.h"


static inline void vectorBlurinner(ui8vector &pixels, ui8vector &zPixels, ui8vector &alpha, ui8vector &zprec, ui8vector &aprec);
static inline void vectorBlurrow(QImage &image, int line, ui8vector &alpha, ui8vector &aprec, ui8vector &zprec);
static inline void vectorAverage2Rows(const qint32 *sourceLine1, const qint32 *sourceLine2, qint32 *destinationLine, int width);

int guidedChunkSize = 2;//16;//2;
int staticChunkSize = 2;//16;//1;
void vectorAlphaBlend(QImage &image, QColor color, bool useThreads)
{
    qreal alpha = color.alphaF();
    __m128i a0 = _mm_set1_epi16((short)((1.0f - alpha) * 255.99f));
    __m128i a1 = _mm_set1_epi16((short)(alpha * 255.99f));
    __m128i lomask = _mm_set1_epi16(0x00ff);
    __m128i himask = _mm_slli_epi16(lomask, 8);


    __m128i blendPixel = _mm_set1_epi32(color.rgb());
    __m128i loword;
    loword = _mm_and_si128(blendPixel, lomask);
    loword = _mm_mullo_epi16(a1, loword);
    loword = _mm_srli_epi16(loword, 8);

    __m128i hiword;
    hiword = _mm_and_si128(blendPixel, himask);
    hiword = _mm_srli_epi16(hiword, 8);
    hiword = _mm_mullo_epi16(a1, hiword);
    hiword = _mm_and_si128(hiword, himask);

    blendPixel = _mm_or_si128(loword, hiword);

    int height = image.height();
    int width = image.width();
    //        #pragma omp parallel for if (useThreads) schedule(guided, 2)
    for (int y = 0; y < height; y++)
    {
        qint32 *linePtr = (qint32 *)image.constScanLine(y);
        for (int x = 0; x < width; x += 4)
        {
            if (x + 4 >= width)
                x = width - 4;

            __m128i pixels = _mm_loadu_si128((__m128i *)&linePtr[x]);

            loword = _mm_and_si128(pixels, lomask);
            loword = _mm_mullo_epi16(a0, loword);
            loword = _mm_srli_epi16(loword, 8);

            hiword = _mm_and_si128(pixels, himask);
            hiword = _mm_srli_epi16(hiword, 8);
            hiword = _mm_mullo_epi16(a0, hiword);
            hiword = _mm_and_si128(hiword, himask);

            pixels = _mm_or_si128(loword, hiword);
            pixels = _mm_add_epi8(pixels, blendPixel);

            _mm_storeu_si128((__m128i *)&linePtr[x], pixels);
        }
    }
}

inline void transpose4x4_SSE(const qint32 *srcPtr, qint32 *dstPtr, const int width, const int height)
{
    __m128i row1 = _mm_loadu_si128((__m128i const *)&srcPtr[0 * width]);
    __m128i row2 = _mm_loadu_si128((__m128i const *)&srcPtr[1 * width]);
    __m128i row3 = _mm_loadu_si128((__m128i const *)&srcPtr[2 * width]);
    __m128i row4 = _mm_loadu_si128((__m128i const *)&srcPtr[3 * width]);

    _MM_TRANSPOSE4_PS(*((__m128 *)&row1), *((__m128 *)&row2), *((__m128 *)&row3), *((__m128 *)&row4));

    _mm_storeu_si128((__m128i *)&dstPtr[0 * height], row1);
    _mm_storeu_si128((__m128i *)&dstPtr[1 * height], row2);
    _mm_storeu_si128((__m128i *)&dstPtr[2 * height], row3);
    _mm_storeu_si128((__m128i *)&dstPtr[3 * height], row4);
}

void vectorTranspose(QImage &imageIn, QImage &imageOut, bool useThreads, int maximumThreads, int chunkSize)
{
    int blockSize = 16;
    qint32 *srcPtr = (qint32 *)imageIn.constBits();
    qint32 *dstPtr = (qint32 *)imageOut.constBits();
    int width = imageIn.width();
    int height = imageIn.height();

    if (chunkSize < 1)
        chunkSize = 1;

    #pragma omp parallel for if (useThreads) num_threads(maximumThreads) schedule(guided, chunkSize)
    for (int y = 0; y < height; y += blockSize)
    {
        int y2Max = y + blockSize < height ? y + blockSize : height;
        for (int x = 0; x < width; x += blockSize)
        {
            int x2Max = x + blockSize < width ? x + blockSize : width;
            for (int y2 = y2Max - blockSize; y2 < y2Max; y2 += 4)
            {
                for (int x2 = x2Max - blockSize; x2 < x2Max; x2 += 4)
                {
                    transpose4x4_SSE((qint32 * const)&srcPtr[y2 * width + x2], (qint32 *)&dstPtr[x2 * height + y2], width, height);
                }
            }
        }
    }
}

inline void transpose4x4_SSE2(const qint32 *srcPtr, qint32 *dstPtr, const int width, const int height, ui8vector &zPixels1, ui8vector &zPixels2, ui8vector &alpha, ui8vector &aprec, ui8vector &zprec)
{
    ui16vector row1;
    row1.v = _mm_loadu_si128((__m128i const *)&srcPtr[0 * width]);
    ui16vector row2;
    row2.v = _mm_loadu_si128((__m128i const *)&srcPtr[1 * width]);
    ui16vector row3;
    row3.v = _mm_loadu_si128((__m128i const *)&srcPtr[2 * width]);
    ui16vector row4;
    row4.v = _mm_loadu_si128((__m128i const *)&srcPtr[3 * width]);

    ui8vector low1;
    ui8vector low2;
    ui8vector high1;
    ui8vector high2;
    ui8vector pixels1;
    ui8vector pixels2;
    ui8vector pixels3;
    ui8vector pixels4;

    low1.v = _mm_srli_epi16(_mm_unpacklo_epi8(row1.v, row1.v), 8);
    low2.v = _mm_srli_epi16(_mm_unpacklo_epi8(row2.v, row2.v), 8);
    pixels1.v = _mm_unpacklo_epi64(low1.v, low2.v);
    vectorBlurinner(pixels1, zPixels1, alpha, aprec, zprec);

    pixels2.v = _mm_unpackhi_epi64(low1.v, low2.v);
    vectorBlurinner(pixels2, zPixels1, alpha, aprec, zprec);

    high1.v = _mm_srli_epi16(_mm_unpackhi_epi8(row1.v, row1.v), 8);
    high2.v = _mm_srli_epi16(_mm_unpackhi_epi8(row2.v, row2.v), 8);
    pixels3.v = _mm_unpacklo_epi64(high1.v, high2.v);
    vectorBlurinner(pixels3, zPixels1, alpha, aprec, zprec);

    pixels4.v = _mm_unpackhi_epi64(high1.v, high2.v);
    vectorBlurinner(pixels4, zPixels1, alpha, aprec, zprec);

    row1.v = _mm_packus_epi16(_mm_unpacklo_epi64(pixels1.v, pixels2.v), _mm_unpacklo_epi64(pixels3.v, pixels4.v));
    row2.v = _mm_packus_epi16(_mm_unpackhi_epi64(pixels1.v, pixels2.v), _mm_unpackhi_epi64(pixels3.v, pixels4.v));


    low1.v = _mm_srli_epi16(_mm_unpacklo_epi8(row3.v, row3.v), 8);
    low2.v = _mm_srli_epi16(_mm_unpacklo_epi8(row4.v, row4.v), 8);
    pixels1.v = _mm_unpacklo_epi64(low1.v, low2.v);
    vectorBlurinner(pixels1, zPixels2, alpha, aprec, zprec);

    pixels2.v = _mm_unpackhi_epi64(low1.v, low2.v);
    vectorBlurinner(pixels2, zPixels2, alpha, aprec, zprec);

    high1.v = _mm_srli_epi16(_mm_unpackhi_epi8(row3.v, row3.v), 8);
    high2.v = _mm_srli_epi16(_mm_unpackhi_epi8(row4.v, row4.v), 8);
    pixels3.v = _mm_unpacklo_epi64(high1.v, high2.v);
    vectorBlurinner(pixels3, zPixels2, alpha, aprec, zprec);

    pixels4.v = _mm_unpackhi_epi64(high1.v, high2.v);
    vectorBlurinner(pixels4, zPixels2, alpha, aprec, zprec);

    row3.v = _mm_packus_epi16(_mm_unpacklo_epi64(pixels1.v, pixels2.v), _mm_unpacklo_epi64(pixels3.v, pixels4.v));
    row4.v = _mm_packus_epi16(_mm_unpackhi_epi64(pixels1.v, pixels2.v), _mm_unpackhi_epi64(pixels3.v, pixels4.v));



    _MM_TRANSPOSE4_PS(*((__m128 *)&row1), *((__m128 *)&row2), *((__m128 *)&row3), *((__m128 *)&row4));

    _mm_storeu_si128((__m128i *)&dstPtr[0 * height], row1.v);
    _mm_storeu_si128((__m128i *)&dstPtr[1 * height], row2.v);
    _mm_storeu_si128((__m128i *)&dstPtr[2 * height], row3.v);
    _mm_storeu_si128((__m128i *)&dstPtr[3 * height], row4.v);

}

inline void transpose4x4_SSE2_3(const qint32 *srcPtr, qint32 *dstPtr, const int width, const int height, ui8vector &zPixels1, ui8vector &zPixels2, ui8vector &alpha, ui8vector &aprec, ui8vector &zprec)
{
    ui16vector row1;
    row1.v = _mm_loadu_si128((__m128i const *)&srcPtr[0 * width]);
    ui16vector row2;
    row2.v = _mm_loadu_si128((__m128i const *)&srcPtr[1 * width]);
    ui16vector row3;
    row3.v = _mm_loadu_si128((__m128i const *)&srcPtr[2 * width]);
    ui16vector row4;
    row4.v = _mm_loadu_si128((__m128i const *)&srcPtr[3 * width]);

    ui8vector low1;
    ui8vector low2;
    ui8vector high1;
    ui8vector high2;
    ui8vector pixels1;
    ui8vector pixels2;
    ui8vector pixels3;
    ui8vector pixels4;

    high1.v = _mm_srli_epi16(_mm_unpackhi_epi8(row1.v, row1.v), 8);
    high2.v = _mm_srli_epi16(_mm_unpackhi_epi8(row2.v, row2.v), 8);
    pixels4.v = _mm_unpackhi_epi64(high1.v, high2.v);
    vectorBlurinner(pixels4, zPixels1, alpha, aprec, zprec);

    pixels3.v = _mm_unpacklo_epi64(high1.v, high2.v);
    vectorBlurinner(pixels3, zPixels1, alpha, aprec, zprec);

    low1.v = _mm_srli_epi16(_mm_unpacklo_epi8(row1.v, row1.v), 8);
    low2.v = _mm_srli_epi16(_mm_unpacklo_epi8(row2.v, row2.v), 8);
    pixels2.v = _mm_unpackhi_epi64(low1.v, low2.v);
    vectorBlurinner(pixels2, zPixels1, alpha, aprec, zprec);

    pixels1.v = _mm_unpacklo_epi64(low1.v, low2.v);
    vectorBlurinner(pixels1, zPixels1, alpha, aprec, zprec);

    row1.v = _mm_packus_epi16(_mm_unpacklo_epi64(pixels1.v, pixels2.v), _mm_unpacklo_epi64(pixels3.v, pixels4.v));
    row2.v = _mm_packus_epi16(_mm_unpackhi_epi64(pixels1.v, pixels2.v), _mm_unpackhi_epi64(pixels3.v, pixels4.v));

    high1.v = _mm_srli_epi16(_mm_unpackhi_epi8(row3.v, row3.v), 8);
    high2.v = _mm_srli_epi16(_mm_unpackhi_epi8(row4.v, row4.v), 8);
    pixels4.v = _mm_unpackhi_epi64(high1.v, high2.v);
    vectorBlurinner(pixels4, zPixels2, alpha, aprec, zprec);

    pixels3.v = _mm_unpacklo_epi64(high1.v, high2.v);
    vectorBlurinner(pixels3, zPixels2, alpha, aprec, zprec);

    low1.v = _mm_srli_epi16(_mm_unpacklo_epi8(row3.v, row3.v), 8);
    low2.v = _mm_srli_epi16(_mm_unpacklo_epi8(row4.v, row4.v), 8);
    pixels2.v = _mm_unpackhi_epi64(low1.v, low2.v);
    vectorBlurinner(pixels2, zPixels2, alpha, aprec, zprec);

    pixels1.v = _mm_unpacklo_epi64(low1.v, low2.v);
    vectorBlurinner(pixels1, zPixels2, alpha, aprec, zprec);

    row3.v = _mm_packus_epi16(_mm_unpacklo_epi64(pixels1.v, pixels2.v), _mm_unpacklo_epi64(pixels3.v, pixels4.v));
    row4.v = _mm_packus_epi16(_mm_unpackhi_epi64(pixels1.v, pixels2.v), _mm_unpackhi_epi64(pixels3.v, pixels4.v));

    _mm_storeu_si128((__m128i *)&dstPtr[0 * width], row1.v);
    _mm_storeu_si128((__m128i *)&dstPtr[1 * width], row2.v);
    _mm_storeu_si128((__m128i *)&dstPtr[2 * width], row3.v);
    _mm_storeu_si128((__m128i *)&dstPtr[3 * width], row4.v);

}

void vectorAverage2Rows(const qint32 *sourceLine1, const qint32 *sourceLine2, qint32 *destinationLine, int width)
{
    int newWidth = (width / 8) * 8;
    for (int x = 0; x < newWidth; x += 8)
    {
        __m128i left  = _mm_avg_epu8(_mm_loadu_si128((__m128i const *) &sourceLine1[x]), _mm_loadu_si128((__m128i const *) &sourceLine2[x]));
        __m128i right = _mm_avg_epu8(_mm_loadu_si128((__m128i const *) &sourceLine1[x + 4]), _mm_loadu_si128((__m128i const *) &sourceLine2[x + 4]));

        __m128i t0 = _mm_unpacklo_epi32(left, right);
        __m128i t1 = _mm_unpackhi_epi32(left, right);
        __m128i shuffle1 = _mm_unpacklo_epi32(t0, t1);
        __m128i shuffle2 = _mm_unpackhi_epi32(t0, t1);

        _mm_storeu_si128((__m128i *) &destinationLine[x / 2], _mm_avg_epu8(shuffle1, shuffle2));
    }

    if ((width - newWidth) != 0)
    {
        __m128i left  = _mm_avg_epu8(_mm_loadu_si128((__m128i const *) &sourceLine1[width - 8]), _mm_loadu_si128((__m128i const *) &sourceLine2[width - 8]));
        __m128i right = _mm_avg_epu8(_mm_loadu_si128((__m128i const *) &sourceLine1[width - 4]), _mm_loadu_si128((__m128i const *) &sourceLine2[width - 4]));

        __m128i t0 = _mm_unpacklo_epi32(left, right);
        __m128i t1 = _mm_unpackhi_epi32(left, right);
        __m128i shuffle1 = _mm_unpacklo_epi32(t0, t1);
        __m128i shuffle2 = _mm_unpackhi_epi32(t0, t1);

        int address = (((width - 1) / 2) + 1) - 4;
        _mm_storeu_si128((__m128i *) &destinationLine[address], _mm_avg_epu8(shuffle1, shuffle2));
    }
}

void vectorInplaceImageScale0_5(QImage &imageIn, QImage &imageOut, bool &oddX, bool &oddY, bool useThreads, int maximumThreads, int chunkSize)
{
    int height = imageIn.height();
    height = height / 2;

    if ((height * 2) != imageIn.height())
        oddY = true;
    else
        oddY = false;

    int width = imageIn.width();
    width = width / 2;

    if ((width * 2) != imageIn.width())
        oddX = true;
    else
        oddX = false;

    imageOut = QImage(const_cast<uchar *>(imageIn.constBits()), oddX ? width + 1 : width, oddY ? height + 1 : height, imageIn.format());

    height = height * 2;
    width = imageIn.width();

    if (chunkSize < 1)
        chunkSize = 1;

    int threadlessCount = maximumThreads * chunkSize * 2;
    for (int y = 0; y < threadlessCount; y += 2)
    {
        const qint32 *sourceLine1 = (const qint32 *) imageIn.constScanLine(y);
        const qint32 *sourceLine2 = (const qint32 *) imageIn.constScanLine(y + 1);
        qint32 *destinationLine = (qint32 *) imageOut.constScanLine(y / 2);
        vectorAverage2Rows(sourceLine1, sourceLine2, destinationLine, width);
    }

    #pragma omp parallel for ordered if (useThreads) num_threads(maximumThreads) schedule(static, chunkSize) //scheduler moet static zijn ivm het overschrijven van lijnen die door een ander thread nog ingelezen moeten worden.
    for (int y = threadlessCount; y < height; y += 2)
    {
        const qint32 *sourceLine1 = (const qint32 *) imageIn.constScanLine(y);
        const qint32 *sourceLine2 = (const qint32 *) imageIn.constScanLine(y + 1);
        qint32 *destinationLine = (qint32 *) imageOut.constScanLine(y / 2);
        vectorAverage2Rows(sourceLine1, sourceLine2, destinationLine, width);
    }

    if (oddY)
    {
        const qint32 *sourceLine1 = (const qint32 *) imageIn.constScanLine(height - 1);
        const qint32 *sourceLine2 = (const qint32 *) imageIn.constScanLine(height);
        qint32 *destinationLine = (qint32 *) imageOut.constScanLine(height / 2);
        vectorAverage2Rows(sourceLine1, sourceLine2, destinationLine, width);
    }
}

void vectorImageScale0_5(QImage &imageIn, QImage &imageOut, bool &oddX, bool &oddY, bool useThreads)
{
    int height = imageIn.height();
    height = height / 2;

    if ((height * 2) != imageIn.height())
        oddY = true;
    else
        oddY = false;

    int width = imageIn.width();
    width = width / 2;

    if ((width * 2) != imageIn.width())
        oddX = true;
    else
        oddX = false;

    imageOut = QImage(oddX ? width + 1 : width, oddY ? height + 1 : height, imageIn.format());

    height = height * 2;
    width = imageIn.width();

    #pragma omp parallel for if (useThreads) ordered schedule(guided, guidedChunkSize)
    for (int y = 0; y < height; y += 2)
    {
        const qint32 *sourceLine1 = (const qint32 *) imageIn.constScanLine(y);
        const qint32 *sourceLine2 = (const qint32 *) imageIn.constScanLine(y + 1);
        qint32 *destinationLine = (qint32 *) imageOut.constScanLine(y / 2);
        vectorAverage2Rows(sourceLine1, sourceLine2, destinationLine, width);
    }

    if (oddY)
    {
        const qint32 *sourceLine1 = (const qint32 *) imageIn.constScanLine(height - 1);
        const qint32 *sourceLine2 = (const qint32 *) imageIn.constScanLine(height);
        qint32 *destinationLine = (qint32 *) imageOut.constScanLine(height / 2);
        vectorAverage2Rows(sourceLine1, sourceLine2, destinationLine, width);
    }
}

void vectorInplaceImageScale2(QImage &imageIn, QImage &imageOut, bool oddX, bool oddY, bool useThreads, int maximumThreads, int chunkSize)
{
    int height = imageIn.height();
    int width = imageIn.width();

    imageOut = QImage(const_cast<uchar *>(imageIn.constBits()), oddX ? (width * 2) - 1 : width * 2, oddY ? (height * 2) - 1 : height * 2, imageIn.format());

    if (oddY)
        height -= 1;

    if (oddX)
        width -= 1;

    if (oddY)
    {
        qint32 *oldLine = (qint32 *) imageIn.constScanLine(height);
        qint32 *newLine = (qint32 *) imageOut.constScanLine(height + height);
        qint32 *ptr = newLine;

        for (int x = 0; x < width; x++)
        {
            *ptr++ = *oldLine;
            *ptr++ = *oldLine++;
        }

        if (oddX)
            *ptr = *(oldLine - 1);
    }

    if (chunkSize < 1)
        chunkSize = 1;

    int threadlessCount = maximumThreads * chunkSize;

    #pragma omp parallel for ordered if (useThreads) num_threads(maximumThreads) schedule(guided, chunkSize)
    for (int y = height - 1; y >= threadlessCount; y--)
    {
        int outputLineNumber = y * 2;
        qint32 *oldLine = (qint32 *) imageIn.constScanLine(y);
        qint32 *newLine1 = (qint32 *) imageOut.constScanLine(outputLineNumber);
        qint32 *newLine2 = (qint32 *) imageOut.constScanLine(outputLineNumber + 1);

        qint32 *ptr = newLine1;
        for (int x = 0; x < width; x++)
        {
            *ptr++ = *oldLine;
            *ptr++ = *oldLine++;
        }

        if (oddX)
            *ptr = *(oldLine - 1);

        memcpy(newLine2, newLine1, sizeof(*newLine1) * imageOut.width());
    }

    for (int y = threadlessCount - 1; y >= 0; y--)
    {
        int outputLineNumber = y * 2;
        qint32 *oldLine = (qint32 *) imageIn.constScanLine(y);
        qint32 *newLine1 = (qint32 *) imageOut.constScanLine(outputLineNumber);
        qint32 *newLine2 = (qint32 *) imageOut.constScanLine(outputLineNumber + 1);

        qint32 *ptr = newLine1;
        for (int x = 0; x < width; x++)
        {
            *ptr++ = *oldLine;
            *ptr++ = *oldLine++;
        }

        if (oddX)
            *ptr = *(oldLine - 1);

        memcpy(newLine2, newLine1, sizeof(*newLine1) * imageOut.width());
    }

}

void vectorImageScale2(QImage &imageIn, QImage &imageOut, bool oddX, bool oddY, bool useThreads)
{
    int height = imageIn.height();
    int width = imageIn.width();

    imageOut = QImage(oddX ? (width * 2) - 1 : width * 2, oddY ? (height * 2) - 1 : height * 2, imageIn.format());

    if (oddY)
        height -= 1;

    if (oddX)
        width -= 1;

    #pragma omp parallel for if (useThreads) ordered schedule(guided, guidedChunkSize)
    for (int y = 0; y < height; y++)
    {
        int outputLineNumber = y * 2;
        qint32 *oldLine = (qint32 *) imageIn.constScanLine(y);
        qint32 *newLine1 = (qint32 *) imageOut.constScanLine(outputLineNumber);
        qint32 *newLine2 = (qint32 *) imageOut.constScanLine(outputLineNumber + 1);

        qint32 *ptr = newLine1;
        for (int x = 0; x < width; x++)
        {
            *ptr++ = *oldLine;
            *ptr++ = *oldLine++;
        }

        if (oddX)
            *ptr = *(oldLine - 1);

        memcpy(newLine2, newLine1, sizeof(*newLine1) * imageOut.width());
    }

    if (oddY)
    {
        qint32 *oldLine = (qint32 *) imageIn.constScanLine(height);
        qint32 *newLine = (qint32 *) imageOut.constScanLine(height + height);
        qint32 *ptr = newLine;

        for (int x = 0; x < width; x++)
        {
            *ptr++ = *oldLine;
            *ptr++ = *oldLine++;
        }

        if (oddX)
            *ptr = *(oldLine - 1);
    }
}

static int get_max_threads(void)
{
#ifdef omp_get_max_threads
    return omp_get_max_threads();
#else
    return 1;
#endif
}

void vectorExponentialBlur(QImage &image, int radius, bool useThreads, const QColor &color)
{
    if (image.isNull())
        return;

    if (radius < 1)
        return;

    if (image.width() < 16)
        return;

    if (image.height() < 16)
        return;

    bool scale = false;
    if ((image.height() > 45 && image.width() > 45) && (radius / 2) >= 2)
    {
        radius /= 2;
        scale = true;
    }

    int maximumThreads = get_max_threads();
    if (hyperThreadingEnabled)
        maximumThreads /= 2;

    if (maximumThreads < 1 || (image.size().width() * image.size().height()) < 250000)
    {
        maximumThreads = 1;
        useThreads = false;
    }

    int horizontalChunkSize = 256 / image.width();
    if (scale)
        horizontalChunkSize *= 2;

    if (horizontalChunkSize < 1)
        horizontalChunkSize = 1;

    int verticalChunkSize = 256 / image.height();
    if (scale)
        verticalChunkSize *= 2;

    if (verticalChunkSize < 1)
        verticalChunkSize = 1;

    ui8vector aprec;
    aprec.v = _mm_set1_epi16((quint16) 14);
    ui8vector zprec;
    zprec.v = _mm_set1_epi16((quint16) 6);

    ui8vector alpha;
    alpha.v = _mm_set1_epi16((quint16)((1 << aprec.i[0]) * (1.0f - expf(-2.3f / (radius + 1.f)))));

    QImage scaledImage;
    bool oddX;
    bool oddY;
    if (scale)
        vectorInplaceImageScale0_5(image, scaledImage, oddX, oddY, useThreads, maximumThreads, horizontalChunkSize / 2);
    else
        scaledImage = image;


    int scaledHeight = scaledImage.height();
    #pragma omp parallel for if (useThreads) num_threads(maximumThreads) schedule(guided, horizontalChunkSize)
    for (int row = 0; row < scaledHeight; row += 2)
    {
        vectorBlurrow(scaledImage, row, alpha, aprec, zprec);
    }

    QImage transposedImage = QImage(scaledImage.height(), scaledImage.width(), scaledImage.format());
    vectorTranspose(scaledImage, transposedImage, useThreads, maximumThreads, horizontalChunkSize);

    int transposedHeight = transposedImage.height();
    #pragma omp parallel for if (useThreads) num_threads(maximumThreads) schedule(guided, verticalChunkSize)
    for (int row = 0; row < transposedHeight; row += 2)
    {
        vectorBlurrow(transposedImage, row, alpha, aprec, zprec);
    }

    QImage transposedImage2(const_cast<uchar *>(image.constBits()), transposedImage.height(), transposedImage.width(), transposedImage.format());
    vectorTranspose(transposedImage, transposedImage2, useThreads, maximumThreads, verticalChunkSize);

    if (color != Qt::transparent)
    {
        vectorAlphaBlend(transposedImage2, color, useThreads);
    }

    QImage imageOut;
    if (scale)
        vectorInplaceImageScale2(transposedImage2, imageOut, oddX, oddY, useThreads, maximumThreads, horizontalChunkSize);
}

static inline void vectorBlurinner(ui8vector &pixels, ui8vector &zPixels, ui8vector &alpha, ui8vector &aprec, ui8vector &zprec)
{
    ui8vector result;
    result.v = _mm_slli_epi16(pixels.v, zprec.i[0]);
    result.v = _mm_sub_epi16(result.v, zPixels.v);

    ui8vector loMul, hiMul;
    loMul.v = _mm_mullo_epi16(result.v, alpha.v);
    hiMul.v = _mm_mulhi_epi16(result.v, alpha.v);

    ui4vector loResult, hiResult;
    loResult.v = _mm_unpacklo_epi16(loMul.v, hiMul.v);
    loResult.v = _mm_srai_epi32(loResult.v, aprec.i[0]);
    hiResult.v = _mm_unpackhi_epi16(loMul.v, hiMul.v);
    hiResult.v = _mm_srai_epi32(hiResult.v, aprec.i[0]);
    result.v = _mm_packs_epi32(loResult.v, hiResult.v);

    zPixels.v = _mm_add_epi16(zPixels.v, result.v);
    pixels.v = _mm_srai_epi16(zPixels.v, zprec.i[0]);
}

static inline void vectorBlurrow(QImage &image, int line, ui8vector &alpha, ui8vector &aprec, ui8vector &zprec)
{
    quint32 *linePtr1 = (quint32 *)image.constScanLine(line);
    if (!linePtr1)
        return;

    quint32 *linePtr2;
    if ((line + 1) >= image.height())
        linePtr2 = linePtr1;
    else
        linePtr2 = (quint32 *) image.constScanLine(line + 1);

    ui16vector vecLine1;
    ui16vector vecLine2;
    ui8vector low1;
    ui8vector low2;
    ui8vector high1;
    ui8vector high2;
    ui8vector pixels1;
    ui8vector pixels2;
    ui8vector pixels3;
    ui8vector pixels4;
    ui16vector out1;
    ui16vector out2;

    int width = image.width() - 4;
    int offset = image.width() - (((image.width() - 1) / 4) * 4);
    if (offset <= 1)
        offset = 1;
    else if (offset == 3)
        offset = 2;
    else
        offset = offset / 2;

    uchar *line1 = (uchar *) &linePtr1[offset - 1];
    uchar *line2 = (uchar *) &linePtr2[offset - 1];

    ui8vector zPixels;
    zPixels.v = _mm_setr_epi16(*(line1), *(line1 + 1), *(line1 + 2), *(line1 + 3), *(line2), *(line2 + 1), *(line2 + 2), *(line2 + 3));
    zPixels.v = _mm_slli_epi16(zPixels.v, zprec.i[0]);

    int index;
    for (index = offset; index <= width; index += 4)
    {
        vecLine1.v = _mm_loadu_si128((const __m128i *) &linePtr1[index]);
        vecLine2.v = _mm_loadu_si128((const __m128i *) &linePtr2[index]);

        low1.v = _mm_srli_epi16(_mm_unpacklo_epi8(vecLine1.v, vecLine1.v), 8);
        low2.v = _mm_srli_epi16(_mm_unpacklo_epi8(vecLine2.v, vecLine2.v), 8);
        pixels1.v = _mm_unpacklo_epi64(low1.v, low2.v);
        vectorBlurinner(pixels1, zPixels, alpha, aprec, zprec);

        pixels2.v = _mm_unpackhi_epi64(low1.v, low2.v);
        vectorBlurinner(pixels2, zPixels, alpha, aprec, zprec);

        high1.v = _mm_srli_epi16(_mm_unpackhi_epi8(vecLine1.v, vecLine1.v), 8);
        high2.v = _mm_srli_epi16(_mm_unpackhi_epi8(vecLine2.v, vecLine2.v), 8);
        pixels3.v = _mm_unpacklo_epi64(high1.v, high2.v);
        vectorBlurinner(pixels3, zPixels, alpha, aprec, zprec);

        pixels4.v = _mm_unpackhi_epi64(high1.v, high2.v);
        vectorBlurinner(pixels4, zPixels, alpha, aprec, zprec);

        out1.v = _mm_packus_epi16(_mm_unpacklo_epi64(pixels1.v, pixels2.v), _mm_unpacklo_epi64(pixels3.v, pixels4.v));
        out2.v = _mm_packus_epi16(_mm_unpackhi_epi64(pixels1.v, pixels2.v), _mm_unpackhi_epi64(pixels3.v, pixels4.v));

        _mm_storeu_si128((__m128i *) &linePtr1[index], out1.v);
        _mm_storeu_si128((__m128i *) &linePtr2[index], out2.v);
    }

    for (index -= 5; index >= 0; index -= 4)
    {
        vecLine1.v = _mm_loadu_si128((const __m128i *) &linePtr1[index]);
        vecLine2.v = _mm_loadu_si128((const __m128i *) &linePtr2[index]);

        high1.v = _mm_srli_epi16(_mm_unpackhi_epi8(vecLine1.v, vecLine1.v), 8);
        high2.v = _mm_srli_epi16(_mm_unpackhi_epi8(vecLine2.v, vecLine2.v), 8);
        pixels4.v = _mm_unpackhi_epi64(high1.v, high2.v);
        vectorBlurinner(pixels4, zPixels, alpha, aprec, zprec);

        pixels3.v = _mm_unpacklo_epi64(high1.v, high2.v);
        vectorBlurinner(pixels3, zPixels, alpha, aprec, zprec);

        low1.v = _mm_srli_epi16(_mm_unpacklo_epi8(vecLine1.v, vecLine1.v), 8);
        low2.v = _mm_srli_epi16(_mm_unpacklo_epi8(vecLine2.v, vecLine2.v), 8);
        pixels2.v = _mm_unpackhi_epi64(low1.v, low2.v);
        vectorBlurinner(pixels2, zPixels, alpha, aprec, zprec);

        pixels1.v = _mm_unpacklo_epi64(low1.v, low2.v);
        vectorBlurinner(pixels1, zPixels, alpha, aprec, zprec);

        out1.v = _mm_packus_epi16(_mm_unpacklo_epi64(pixels1.v, pixels2.v), _mm_unpacklo_epi64(pixels3.v, pixels4.v));
        out2.v = _mm_packus_epi16(_mm_unpackhi_epi64(pixels1.v, pixels2.v), _mm_unpackhi_epi64(pixels3.v, pixels4.v));

        _mm_storeu_si128((__m128i *) &linePtr1[index], out1.v);
        _mm_storeu_si128((__m128i *) &linePtr2[index], out2.v);
    }
}

GraphicsBlurEffect::GraphicsBlurEffect(QObject *parent) : QGraphicsEffect(parent)
{
    radius = 6;
}

GraphicsBlurEffect::~GraphicsBlurEffect()
{
}

qreal GraphicsBlurEffect::blurRadius() const
{
    return radius;
}

void GraphicsBlurEffect::setBlurRadius(qreal blurRadius)
{
    radius = blurRadius;
    updateBoundingRect();
    emit blurRadiusChanged(blurRadius);
}

void GraphicsBlurEffect::setBlurRadius(int blurRadius)
{
    setBlurRadius((qreal) blurRadius);
}

QRectF GraphicsBlurEffect::boundingRectFor(const QRectF &rect) const
{
    const qreal delta = (1.0 * radius) + 1;
    return rect.adjusted(-delta, -delta, delta, delta);
}

void GraphicsBlurEffect::draw(QPainter *painter)
{
    if (blurRadius() < 1)
    {
        drawSource(painter);
        return;
    }

    QPoint offset;
    if (sourceIsPixmap())
    {
        const QPixmap pixmap = sourcePixmap(Qt::LogicalCoordinates, &offset, QGraphicsEffect::PadToEffectiveBoundingRect);
        if (!pixmap.isNull())
        {
            QString key("GraphicsBlurEffect:" + QString::number(blurRadius()) + QString::number(boundingRect().width()) + QString::number(pixmap.rect().height()));
            QPixmap *found = QPixmapCache::find(key);
            if (found)
                painter->drawPixmap(boundingRect(), *found, found->rect());
            else
            {
                QImage image = pixmap.toImage();
                vectorExponentialBlur(image, (int) radius, true);
                QPixmap blurredPixmap =  QPixmap::fromImage(image);
                painter->drawPixmap(boundingRect(), blurredPixmap, blurredPixmap.rect());
                QPixmapCache::insert(key, blurredPixmap);
            }
        }

        return;
    }

    const QPixmap pixmap = sourcePixmap(Qt::DeviceCoordinates, &offset, QGraphicsEffect::PadToEffectiveBoundingRect);
    if (pixmap.isNull())
        return;

    QTransform restoreTransform = painter->worldTransform();
    painter->setWorldTransform(QTransform());
    QImage image = pixmap.toImage();
    vectorExponentialBlur(image, (int) radius, true);
    painter->drawImage(boundingRect(), image);
    painter->setWorldTransform(restoreTransform);
}
