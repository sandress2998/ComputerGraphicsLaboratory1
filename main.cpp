#include <iostream>
#include <vector>
#include <cmath>
#include <cstdio>
#include <png.h>
#include <cstdint>
#include <cstring>

/// ГЕНЕРАЦИЯ КРУГА

// Создаёт изображение w×h с круглым полутоновым объектом
// Яркость убывает от центра к краю по косинусоидальному профилю
std::vector<uint8_t> generate_circle(int w, int h) {
    std::vector<uint8_t> img(w * h, 0);  // Инициализируем чёрным фоном

    // Центр круга (в пиксельных координатах)
    float cx = (w - 1) * 0.5f;
    float cy = (h - 1) * 0.5f;

    // Радиус круга (45% от меньшей стороны)
    float r = std::min(w, h) * 0.45f;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            // Расстояние от центра до текущего пикселя
            float dx = x - cx;
            float dy = y - cy;
            float dist = std::sqrt(dx * dx + dy * dy);

            // Нормируем расстояние: t = 0 в центре, t = 1 на границе круга
            float t = dist / r;

            if (t <= 1.0f) {  // Только внутри круга
                // Плавный косинусоидальный профиль яркости
                // cos(0) = 1 (ярко в центре), cos(π/2) = 0 (темно на краю)
                float v = std::cos(t * 3.14159265f * 0.5f);
                if (v < 0.f) v = 0.f;  // На всякий случай обрезаем отрицательные

                // Преобразуем в диапазон 0-255
                img[y * w + x] = static_cast<uint8_t>(std::round(255.f * v));
            }
            // За пределами круга остаётся 0 (чёрный фон)
        }
    }
    return img;
}

/// Генерация тестовых изображений


std::vector<uint8_t> generate_gradient_diagonal(int w, int h) {
    std::vector<uint8_t> img(w * h);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float t = (x + y) / float((w - 1) + (h - 1));
            img[y * w + x] = static_cast<uint8_t>(std::round(255.f * t));
        }
    }
    return img;
}

std::vector<uint8_t> generate_gradient_horizontal(int w, int h) {
    std::vector<uint8_t> img(w * h);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float t = x / float(w - 1);
            img[y * w + x] = static_cast<uint8_t>(std::round(255.f * t));
        }
    }
    return img;
}

// Радиальный градиент: от белого в центре к чёрному по краям
std::vector<uint8_t> generate_gradient_radial(int w, int h) {
    std::vector<uint8_t> img(w * h);
    float cx = (w - 1) * 0.5f;
    float cy = (h - 1) * 0.5f;
    float max_dist = std::sqrt(cx * cx + cy * cy);  // Расстояние до угла

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float dx = x - cx;
            float dy = y - cy;
            float dist = std::sqrt(dx * dx + dy * dy);
            float t = dist / max_dist;  // 0 в центре, ~1 на краях
            if (t > 1.0f) t = 1.0f;

            // Инвертируем: 1 в центре → 0 на краях
            img[y * w + x] = static_cast<uint8_t>(std::round(255.f * (1.0f - t)));
        }
    }
    return img;
}


// Радиальная альфа-маска: 0 в центре, 255 на краях
std::vector<uint8_t> generate_alpha_radial(int w, int h) {
    std::vector<uint8_t> img(w * h);
    float cx = (w - 1) * 0.5f;
    float cy = (h - 1) * 0.5f;
    float r = std::sqrt(cx * cx + cy * cy);  // Максимальное расстояние до угла

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float dx = x - cx;
            float dy = y - cy;
            float t = std::sqrt(dx * dx + dy * dy) / r;  // 0..~1
            if (t > 1.f) t = 1.f;  // Обрезаем выход за пределы
            img[y * w + x] = static_cast<uint8_t>(std::round(255.f * t));
        }
    }
    return img;
}

// Создает маску с равномерной прозрачностью 0.5 (50%)
// w, h - ширина и высота маски
// возвращает вектор с значениями 128 (50% от 255)
std::vector<uint8_t> generate_uniform_alpha_mask(int w, int h) {
    std::vector<uint8_t> mask(w * h, 128);  // 128 = 255 * 0.5 = 50% прозрачности
    return mask;
}

/// Альфа-смешивание

// Смешивает два изображения A и B с весами из Alpha
// Формула: out = ((255 - alpha) * A + alpha * B) / 255
// При alpha=0: out=A (показываем только A)
// При alpha=255: out=B (показываем только B)
// При alpha=128: out=(A+B)/2 (50/50)
std::vector<uint8_t> blend_gray8(const std::vector<uint8_t>& A,
                                 const std::vector<uint8_t>& B,
                                 const std::vector<uint8_t>& Alpha,
                                 int w, int h) {
    std::vector<uint8_t> out(w * h);
    for (int i = 0; i < w * h; ++i) {
        int a = Alpha[i];
        int a_inv = 255 - a;
        // +127 для корректного округления при делении на 255
        out[i] = static_cast<uint8_t>((a_inv * A[i] + a * B[i] + 127) / 255);
    }
    return out;
}

// Проверка размеров изображений, который будут накладываться
int checkIfSizesEquals(int wA, int hA, int wB, int hB, int wAlpha, int hAlpha) {
    if (wA != wB || hA != hB || wAlpha != wA || hAlpha != hA) {
        std::cerr << "Error: image sizes aren't equal!\n";
        std::cerr << "  Image A: " << wA << "x" << hA << "\n";
        std::cerr << "  Image B: " << wB << "x" << hB << "\n";
        std::cerr << "  Alpha: " << wAlpha << "x" << hAlpha << "\n";
        return 1;
    }
    return 0;
}

// Проверка размеров изображений, который будут накладываться
int checkIfSizesEquals(int wA, int hA, int wB, int hB) {
    if (wA != wB || hA != hB) {
        std::cerr << "Error: image sizes aren't equal!\n";
        std::cerr << "  Image A: " << wA << "x" << hA << "\n";
        std::cerr << "  Image B: " << wB << "x" << hB << "\n";
        return 1;
    }
    return 0;
}

static void read_cb(png_structp png_ptr, png_bytep data, png_size_t len) {
    FILE* fp = reinterpret_cast<FILE*>(png_get_io_ptr(png_ptr));
    if (!fp) png_error(png_ptr, "no file");
    size_t n = std::fread(data, 1, len, fp);
    if (n != len) png_error(png_ptr, "short read");
}

void read_png_gray8(const char* path, std::vector<unsigned char>& img, int& w, int& h) {
    FILE* fp = std::fopen(path, "rb");
    if (!fp) throw std::runtime_error("fopen failed");

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) { std::fclose(fp); throw std::runtime_error("create_read_struct failed"); }
    png_infop info = png_create_info_struct(png);
    if (!info) { png_destroy_read_struct(&png, nullptr, nullptr); std::fclose(fp); throw std::runtime_error("create_info_struct failed"); }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, nullptr);
        std::fclose(fp);
        throw std::runtime_error("libpng read error");
    }

    png_set_read_fn(png, fp, read_cb);
    png_read_info(png, info);

    png_uint_32 width, height;
    int bit_depth, color_type;
    png_get_IHDR(png, info, &width, &height, &bit_depth, &color_type, nullptr, nullptr, nullptr);

    // Сохраняем информацию о прозрачности
    bool has_alpha = (color_type & PNG_COLOR_MASK_ALPHA) || png_get_valid(png, info, PNG_INFO_tRNS);

    // ПРЕОБРАЗОВАНИЯ С СОХРАНЕНИЕМ ПРОЗРАЧНОСТИ:

    // 1. Если палитра - конвертируем в RGB
    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png);

    // 2. Обрабатываем tRNS (прозрачность в палитровых и grayscale изображениях)
    if (png_get_valid(png, info, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(png);
        has_alpha = true;
    }

    // 3. Если меньше 8 бит - расширяем до 8 бит
    if (bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png);

    // 4. Если 16 бит - понижаем до 8 бит
    if (bit_depth == 16)
        png_set_strip_16(png);

    png_read_update_info(png, info);

    // Получаем обновленные параметры
    int final_channels = png_get_channels(png, info);
    png_get_IHDR(png, info, &width, &height, &bit_depth, &color_type, nullptr, nullptr, nullptr);

    w = static_cast<int>(width);
    h = static_cast<int>(height);

    // Временный буфер для чтения данных (с альфой если есть)
    png_size_t rowbytes = png_get_rowbytes(png, info);
    std::vector<unsigned char> scan(rowbytes);
    img.assign(static_cast<size_t>(w) * h, 0);

    // Читаем и конвертируем с учетом прозрачности
    for (int y = 0; y < h; ++y) {
        png_read_row(png, scan.data(), nullptr);
        unsigned char* dst = &img[static_cast<size_t>(y) * w];

        if (final_channels == 4) { // RGBA
            const unsigned char* p = scan.data();
            for (int x = 0; x < w; ++x) {
                unsigned char r = p[0], g = p[1], b = p[2], a = p[3];
                // Если пиксель полностью прозрачный - делаем его черным (0)
                if (a == 0) {
                    dst[x] = 0;
                } else {
                    // Конвертируем в grayscale с учетом альфа-канала
                    int yv = (77 * r + 150 * g + 29 * b + 128) >> 8;
                    dst[x] = static_cast<unsigned char>(yv);
                }
                p += 4;
            }
        }
        else if (final_channels == 3) { // RGB (нет прозрачности)
            const unsigned char* p = scan.data();
            for (int x = 0; x < w; ++x) {
                unsigned char r = p[0], g = p[1], b = p[2];
                int yv = (77 * r + 150 * g + 29 * b + 128) >> 8;
                dst[x] = static_cast<unsigned char>(yv);
                p += 3;
            }
        }
        else if (final_channels == 2) { // GRAY+ALPHA
            const unsigned char* p = scan.data();
            for (int x = 0; x < w; ++x) {
                unsigned char gray = p[0], alpha = p[1];
                // Если прозрачный - черный, иначе берем значение яркости
                dst[x] = (alpha == 0) ? 0 : gray;
                p += 2;
            }
        }
        else if (final_channels == 1) { // GRAY (нет прозрачности)
            std::memcpy(dst, scan.data(), static_cast<size_t>(w));
        }
        else {
            png_error(png, "unsupported channels count");
        }
    }

    png_read_end(png, nullptr);
    png_destroy_read_struct(&png, &info, nullptr);
    std::fclose(fp);
}

/// Колбэки для работы с файлами через наш рантайм
/* Вместо того чтобы libpng сама работала с файлом (png_init_io),
 * мы даём ей колбэки, которые она будет вызывать для записи/чтения данных.
*/

static void write_cb(png_structp png_ptr, png_bytep data, png_size_t len) {
    FILE* fp = reinterpret_cast<FILE*>(png_get_io_ptr(png_ptr));
    if (!fp) png_error(png_ptr, "no file");
    size_t n = std::fwrite(data, 1, len, fp);
    if (n != len) png_error(png_ptr, "short write");
}

static void flush_cb(png_structp png_ptr) {
    FILE* fp = reinterpret_cast<FILE*>(png_get_io_ptr(png_ptr));
    if (fp) std::fflush(fp);
}

void write_png_gray8(const char* path, const std::vector<unsigned char>& img, int w, int h) {
    if (w <= 0 || h <= 0) throw std::runtime_error("bad dims");
    if (img.size() != static_cast<size_t>(w) * h) throw std::runtime_error("size mismatch");

    FILE* fp = std::fopen(path, "wb");
    if (!fp) throw std::runtime_error("fopen wb failed");

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) { std::fclose(fp); throw std::runtime_error("create_write_struct failed"); }

    png_infop info = png_create_info_struct(png);
    if (!info) { png_destroy_write_struct(&png, nullptr); std::fclose(fp); throw std::runtime_error("create_info_struct failed"); }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_write_struct(&png, nullptr);
        std::fclose(fp);
        throw std::runtime_error("libpng write error");
    }

    png_set_write_fn(png, fp, write_cb, flush_cb);

    png_set_IHDR(png, info,
                 static_cast<png_uint_32>(w),
                 static_cast<png_uint_32>(h),
                 8, PNG_COLOR_TYPE_GRAY,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE,
                 PNG_FILTER_TYPE_BASE);

    png_write_info(png, info);

    for (int y = 0; y < h; ++y) {
        const unsigned char* src = &img[static_cast<size_t>(y) * w];
        png_bytep row = const_cast<png_bytep>(src);
        png_write_row(png, row);
    }

    png_write_end(png, nullptr);
    png_destroy_write_struct(&png, nullptr);
    std::fclose(fp);
}

void apply_circle_mask_to_image(const char* input_path, const char* output_path) {
    try {
        std::cout << "Applying circular mask to image: " << input_path << "\n";

        // Читаем исходное изображение
        std::vector<uint8_t> img;
        int w = 0, h = 0;
        read_png_gray8(input_path, img, w, h);
        std::cout << "Read image: " << w << "x" << h << "\n";

        // Создаём круглую маску того же размера
        std::vector<uint8_t> mask(w * h, 0);  // Инициализируем чёрным (0)

        // Центр круга
        float cx = (w - 1) * 0.5f;
        float cy = (h - 1) * 0.5f;

        float r = std::min(w, h) * 0.45f;

        // Заполняем маску: 255 внутри круга, 0 снаружи
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                float dx = x - cx;
                float dy = y - cy;
                float dist = std::sqrt(dx * dx + dy * dy);

                if (dist <= r) {
                    mask[y * w + x] = 255;  // Белый внутри круга
                }
                // Снаружи остаётся 0 (чёрный)
            }
        }

        // Применяем маску: умножаем изображение на маску
        std::vector<uint8_t> result(w * h, 0);
        for (int i = 0; i < w * h; ++i) {
            result[i] = static_cast<uint8_t>((img[i] * mask[i]) / 255);
        }

        // Сохраняем результат
        write_png_gray8(output_path, result, w, h);
        std::cout << "Saved masked image: " << output_path << "\n";
        std::cout << "Circular mask applied successfully!\n\n";

    } catch (const std::exception& e) {
        std::cerr << "Error in task1_circle_mask: " << e.what() << "\n";
        throw;
    }
}

/// ЗАДАНИЕ 1: Круглое полутоновое изображение
void task1_generating_halftone_circle() {
    const int W = 512; // Ширина изображения
    const int H = 512; // Высота изображения

    std::cout << "Generating a circular halftone image...\n";
    auto circle = generate_circle(W, H);
    write_png_gray8("circle.png", circle, W, H);
    std::cout << "Saved in circle.png\n";

    std::cout << "\nChecking: reading circle.png back...\n";
    std::vector<uint8_t> test_img;
    int rw = 0, rh = 0;
    read_png_gray8("circle.png", test_img, rw, rh);
    std::cout << "Readed back: " << rw << "x" << rh << "\n";

    std::cout << "\nTASK 1 DONE!\n";
    std::cout << "Created:\n";
    std::cout << "  - circle.png (circular halftone image)\n\n";
}

/// ЗАДАНИЕ 1: Маска в виде круга
void task1_circle_mask() {
    const char* images_paths_input[] = {
            "image1.png",
            "image2.png",
            "image3.png"
    };

    const char* images_paths_output[] = {
            "output_image1.png",
            "output_image2.png",
            "output_image3.png"
    };

    for (int i = 0; i < 3; ++i) {
        apply_circle_mask_to_image(images_paths_input[i], images_paths_output[i]);
    }

}

/// ЗАДАНИЕ 2: Смешивание трёх пар изображений
void task2_blending_synthetic_images() {
    const int W = 512; // Ширина синтетических изображений
    const int H = 512; // Высота синтетических изображений

    // Пути для сохранения результатов
    const char* paths_output[] = {
            "output_blended1.png",
            "output_blended2.png",
            "output_blended3.png"
    };

    // Генерация альфа-канала (один для всех пар)
    const char* path_alpha = "alpha.png";
    std::cout << "GENERATING ALPHA CHANNEL\n";
    std::cout << "Alpha channel generation " << path_alpha << "...\n";
    std::vector<uint8_t> alpha = generate_alpha_radial(W, H);
    write_png_gray8(path_alpha, alpha, W, H);
    std::cout << "Alpha channel is saved\n\n";

    int wAlpha = W;
    int hAlpha = H;

    /// ПАРА 1: Диагональный градиент + Горизонтальный градиент
    std::cout << "PROCESSING PAIR 1\n";
    std::cout << "Generating images for pair 1...\n";
    auto imgA1 = generate_gradient_diagonal(W, H);
    auto imgB1 = generate_gradient_horizontal(W, H);

    // Сохраняем исходные изображения для проверки
    write_png_gray8("input_a1.png", imgA1, W, H);
    write_png_gray8("input_b1.png", imgB1, W, H);
    std::cout << "Generated and saved input_a1.png, input_b1.png\n";

    // Проверяем размеры
    checkIfSizesEquals(W, H, W, H, wAlpha, hAlpha);
    std::cout << "Sizes are equal\n";

    // Смешивание
    std::cout << "Processing alpha blending...\n";
    auto blended1 = blend_gray8(imgA1, imgB1, alpha, W, H);
    write_png_gray8(paths_output[0], blended1, W, H);
    std::cout << "Saved: " << paths_output[0] << "\n\n";

    /// ПАРА 2: Радиальный градиент + Круг
    std::cout << "PROCESSING PAIR 2\n";
    std::cout << "Generating images for pair 2...\n";
    auto imgA2 = generate_gradient_radial(W, H);
    auto imgB2 = generate_circle(W, H);

    write_png_gray8("input_a2.png", imgA2, W, H);
    write_png_gray8("input_b2.png", imgB2, W, H);
    std::cout << "Generated and saved input_a2.png, input_b2.png\n";

    checkIfSizesEquals(W, H, W, H, wAlpha, hAlpha);
    std::cout << "Sizes are equal\n";

    std::cout << "Processing alpha blending...\n";
    auto blended2 = blend_gray8(imgA2, imgB2, alpha, W, H);
    write_png_gray8(paths_output[1], blended2, W, H);
    std::cout << "Saved: " << paths_output[1] << "\n\n";

    /// ПАРА 3: Горизонтальный градиент + Диагональный градиент (обратная пара 1)
    std::cout << "PROCESSING PAIR 3\n";
    std::cout << "Generating images for pair 3...\n";
    auto imgA3 = generate_gradient_horizontal(W, H);
    auto imgB3 = generate_gradient_diagonal(W, H);

    write_png_gray8("input_a3.png", imgA3, W, H);
    write_png_gray8("input_b3.png", imgB3, W, H);
    std::cout << "Generated and saved input_a3.png, input_b3.png\n";

    checkIfSizesEquals(W, H, W, H, wAlpha, hAlpha);
    std::cout << "Sizes are equal\n";

    std::cout << "Processing alpha blending...\n";
    auto blended3 = blend_gray8(imgA3, imgB3, alpha, W, H);
    write_png_gray8(paths_output[2], blended3, W, H);
    std::cout << "Saved: " << paths_output[2] << "\n\n";
}


/// ЗАДАНИЕ 2: Смешивание несинтетических картинок
void task2_blending_non_synthetic_images() {

    const char* images_for_blending_paths_input[] = {
            "image1_for_blending.png",
            "image2_for_blending.png",
            "image3_for_blending.png"
    };

    const char* images_for_blending_paths_output[] = {
            "output_image1_for_blending.png",
            "output_image2_for_blending.png",
            "output_image3_for_blending.png"
    };

    std::vector<uint8_t> image1_for_blending;
    std::vector<uint8_t> image2_for_blending;
    std::vector<uint8_t> image3_for_blending;

    int w1 = 0, h1 = 0, w2 = 0, h2 = 0, w3 = 0, h3 = 0;
    read_png_gray8(images_for_blending_paths_input[0], image1_for_blending, w1, h1);
    read_png_gray8(images_for_blending_paths_input[1], image2_for_blending, w2, h2);
    read_png_gray8(images_for_blending_paths_input[2], image3_for_blending, w3, h3);

    checkIfSizesEquals(w1, h1, w2, h2);
    checkIfSizesEquals(w1, h1, w3, h3);

    std::vector<uint8_t> alpha2 = generate_uniform_alpha_mask(w1, h1);

    auto blended_image1 = blend_gray8(image1_for_blending, image2_for_blending, alpha2, w1, h1);
    auto blended_image2 = blend_gray8(image2_for_blending, image3_for_blending, alpha2, w1, h1);
    auto blended_image3 = blend_gray8(image3_for_blending, image1_for_blending, alpha2, w1, h1);

    write_png_gray8(images_for_blending_paths_output[0], blended_image1, w1, h1);
    write_png_gray8(images_for_blending_paths_output[1], blended_image2, w2, h2);
    write_png_gray8(images_for_blending_paths_output[2], blended_image3, w3, h3);
}

int main() {
    try {
        task1_circle_mask();
        task1_generating_halftone_circle();
        task2_blending_synthetic_images();
        task2_blending_non_synthetic_images();

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
