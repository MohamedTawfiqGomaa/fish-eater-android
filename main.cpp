
// ============================================
// المكتبات الضرورية فقط
// ============================================
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <cmath>
#include <ctime>
#include <cstdio>
#include <vector>

// ============================================
// الثوابت الأساسية
// ============================================
const int WINDOW_WIDTH = 1200;
const int WINDOW_HEIGHT = 600;
const int OCEAN_HEIGHT = 500;
const int NUM_FISH = 20;
const int INITIAL_TIME = 50;
const float FISH_SIZE = 20.0f;
const float COLLISION_RADIUS = 15.0f;
const float FISH_SPEED = 0.5f;
const float PI = 3.1415926f;

// نمو اللاعب
const float GROWTH_INCREMENT = 0.05f;
const float INITIAL_PLAYER_SIZE = 1.0f;
const float MAX_PLAYER_SIZE = 2.5f;

// سلوك الأسماك
const float FLEE_DISTANCE = 200.0f;
const float CHASE_DISTANCE = 250.0f;
const float FLEE_SPEED = 1.5f;
const float CHASE_SPEED = 1.3f;

// الأمواج
const int WAVE_POINTS = 50;
float waveOffset = 0.0f;

// ============================================
// متغيرات اللعبة
// ============================================
bool isGameOver = false;
int score = 0;
int gameTime = INITIAL_TIME;
float playerSizeScale = INITIAL_PLAYER_SIZE;
float prevMouseX = WINDOW_WIDTH / 2.0f;
bool allFishCollected = false;

// ============================================
// دوال عرض النصوص
//شهد
// ============================================
void drawText(const char *str, int x, int y) {
    glRasterPos2d(x, y);
    while (*str) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *str);
        str++;
    }
}

void drawLargeText(const char *str, int x, int y) {
    glRasterPos2d(x, y);
    while (*str) {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *str);
        str++;
    }
}

void drawNumber(int x, int y, int value) {
    char buffer[32];
    sprintf(buffer, "%d", value);
    drawText(buffer, x, y);
}

void drawFloat(int x, int y, float value) {
    char buffer[32];
    sprintf(buffer, "%.2fx", value);
    drawText(buffer, x, y);
}
//^^^^^^^^^^^^^^^^^^^^^^^^^^^

// ============================================
// رسم المحيط مع أمواج واقعية
//محمد سعيد كامل
// ============================================
void drawOcean() {
    // خلفية المحيط بتدرج بسيط
    glBegin(GL_QUADS);
    glColor3f(0.1f, 0.4f, 0.8f);
    glVertex2f(0, 0);
    glVertex2f(WINDOW_WIDTH, 0);
    glColor3f(0.05f, 0.2f, 0.5f);
    glVertex2f(WINDOW_WIDTH, OCEAN_HEIGHT);
    glVertex2f(0, OCEAN_HEIGHT);
    glEnd();

    // رسم أمواج متعددة الطبقات للواقعية

    // الموجة الأولى (الخلفية - بطيئة)
    glColor4f(0.15f, 0.45f, 0.75f, 0.6f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= WAVE_POINTS; i++) {
        float x = (WINDOW_WIDTH / (float)WAVE_POINTS) * i;
        float y = OCEAN_HEIGHT + sin((x * 0.01f) + (waveOffset * 0.5f)) * 8.0f +
                  sin((x * 0.02f) - (waveOffset * 0.3f)) * 4.0f;
        glVertex2f(x, y);
    }
    glEnd();

    // الموجة الثانية (الوسطى - متوسطة)
    glColor4f(0.2f, 0.55f, 0.85f, 0.7f);
    glLineWidth(2.5f);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= WAVE_POINTS; i++) {
        float x = (WINDOW_WIDTH / (float)WAVE_POINTS) * i;
        float y = OCEAN_HEIGHT + sin((x * 0.015f) + (waveOffset * 0.8f)) * 10.0f +
                  sin((x * 0.03f) + (waveOffset * 0.4f)) * 5.0f;
        glVertex2f(x, y);
    }
    glEnd();

    // الموجة الثالثة (الأمامية - سريعة)
    glColor4f(0.25f, 0.65f, 0.95f, 0.9f);
    glLineWidth(3.0f);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= WAVE_POINTS; i++) {
        float x = (WINDOW_WIDTH / (float)WAVE_POINTS) * i;
        float y = OCEAN_HEIGHT + sin((x * 0.02f) + waveOffset) * 12.0f +
                  sin((x * 0.04f) - (waveOffset * 0.7f)) * 6.0f +
                  sin((x * 0.08f) + (waveOffset * 1.5f)) * 3.0f;
        glVertex2f(x, y);
    }
    glEnd();

    // تحريك الأمواج
    waveOffset += 0.03f;
    if (waveOffset > 2.0f * PI * 100.0f) {
        waveOffset = 0.0f;
    }
}
//^^^^^^^^^^^^^^^^^

// ============================================
// فئة السمكة
// ============================================
class Fish {
public:
    bool isPlayer;
    bool isRedFish;
    bool isLarge;      // مبسط: كبير أو صغير فقط
    float x, y;
    float direction;
    float sizeScale;

    Fish(bool player = false) : isPlayer(player) {
        if (isPlayer) {
            x = WINDOW_WIDTH / 2.0f;
            y = WINDOW_HEIGHT / 2.0f;
            sizeScale = INITIAL_PLAYER_SIZE;
            isLarge = false;
            direction = FISH_SPEED;
        } else {
            isRedFish = (rand() % 10 < 3);  // 30% حمراء
            isLarge = (rand() % 10 < 3);     // 30% كبيرة
            sizeScale = isLarge ? 1.5f : 0.9f;
            x = rand() % WINDOW_WIDTH;
            y = rand() % (OCEAN_HEIGHT - 50);
            direction = (rand() % 2) ? FISH_SPEED : -FISH_SPEED;
        }
    }

    float getCollisionRadius() const {
        return COLLISION_RADIUS * sizeScale;
    }

    void draw() {
        // تحديد اللون
        float r, g, b;
        if (isPlayer) {
            r = 0.2f; g = 0.4f; b = 1.0f;  // أزرق
        } else if (isRedFish) {
            r = 1.0f; g = 0.3f; b = 0.3f;  // أحمر
        } else {
            r = 1.0f; g = 0.9f; b = 0.2f;  // أصفر
        }

        float currentScale = isPlayer ? playerSizeScale : sizeScale;
        float dir = (direction < 0) ? -1.0f : 1.0f;
        float bodyLength = FISH_SIZE * 1.8f * currentScale;
        float bodyHeight = FISH_SIZE * 1.2f * currentScale;

        // رسم الجسم
        glColor3f(r, g, b);
        glBegin(GL_POLYGON);
        for (int i = 0; i <= 20; i++) {
            float angle = PI * float(i) / 20.0f;
            float px = x + dir * (bodyLength * 0.4f * cos(angle));
            float py = y + bodyHeight * 0.5f * sin(angle);
            glVertex2f(px, py);
        }
        glEnd();

        // رسم الذيل
        float tailBaseX = x - dir * bodyLength * 0.5f;
        float tailLength = FISH_SIZE * 0.6f * currentScale;
        glColor3f(r * 0.3f, g * 0.8f, b * 0.3f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(tailBaseX, y);
        for (int i = 0; i <= 8; i++) {
            float angle = (PI / 3.0f) * float(i) / 8.0f - (PI / 6.0f);
            float px = tailBaseX - dir * tailLength * cos(angle);
            float py = y + tailLength * sin(angle);
            glVertex2f(px, py);
        }
        glEnd();

        // رسم العين
        float eyeX = x + dir * bodyLength * 0.35f;
        float eyeY = y + bodyHeight * 0.15f;
        float eyeSize = FISH_SIZE * 0.15f * currentScale;

        // بياض العين
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_POLYGON);
        for (int i = 0; i <= 12; i++) {
            float angle = 2.0f * PI * float(i) / 12.0f;
            glVertex2f(eyeX + eyeSize * cos(angle), eyeY + eyeSize * sin(angle));
        }
        glEnd();

        // بؤبؤ العين
        glColor3f(0.0f, 0.0f, 0.0f);
        glBegin(GL_POLYGON);
        for (int i = 0; i <= 12; i++) {
            float angle = 2.0f * PI * float(i) / 12.0f;
            glVertex2f(eyeX + eyeSize * 0.6f * cos(angle), eyeY + eyeSize * 0.6f * sin(angle));
        }
        glEnd();
    }

    void move(float mouseX = 0.0f, float mouseY = 0.0f, float prevX = 0.0f) {
        if (!isPlayer) {
            x += direction;
            if (x > WINDOW_WIDTH) {
                x = 0;
                y = rand() % (OCEAN_HEIGHT - 50);
            }
            if (x < 0) {
                x = WINDOW_WIDTH;
                y = rand() % (OCEAN_HEIGHT - 50);
            }
        } else {
            if (mouseX > prevX) direction = FISH_SPEED;
            else if (mouseX < prevX) direction = -FISH_SPEED;

            x = mouseX;
            y = WINDOW_HEIGHT - mouseY;
        }
    }

    void moveWithBehavior(float playerX, float playerY, float playerRadius) {
        if (isPlayer) return;

        float dx = playerX - x;
        float dy = playerY - y;
        float dist = sqrt(dx * dx + dy * dy);

        float myRadius = getCollisionRadius();
        float stepX = direction;
        float stepY = 0.0f;

        // الهروب من اللاعب الأكبر
        if (playerRadius > myRadius && dist < FLEE_DISTANCE) {
            stepX = -(dx / dist) * FISH_SPEED * FLEE_SPEED;
            stepY = -(dy / dist) * FISH_SPEED * FLEE_SPEED;
            direction = (stepX < 0) ? -FISH_SPEED : FISH_SPEED;
        }
        // مطاردة اللاعب الأصغر
        else if ((isRedFish || isLarge) && playerRadius < myRadius && dist < CHASE_DISTANCE) {
            stepX = (dx / dist) * FISH_SPEED * CHASE_SPEED;
            stepY = (dy / dist) * FISH_SPEED * CHASE_SPEED;
            direction = (stepX < 0) ? -FISH_SPEED : FISH_SPEED;
        }

        x += stepX;
        y += stepY;

        // لف أفقي
        if (x > WINDOW_WIDTH) x = 0;
        if (x < 0) x = WINDOW_WIDTH;

        // تحديد عمودي
        if (y < 20) y = 20;
        if (y > OCEAN_HEIGHT - 20) y = OCEAN_HEIGHT - 20;
    }
};

// ============================================
// كائنات اللعبة
// ============================================
std::vector<Fish> fishArray;
Fish player(true);

// ============================================
// كشف التصادم
//مروه
// ============================================
//
bool checkCollision(const Fish& a, const Fish& b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float distSq = dx * dx + dy * dy;

    float radiusA = a.isPlayer ? (COLLISION_RADIUS * playerSizeScale) : a.getCollisionRadius();
    float radiusB = b.isPlayer ? (COLLISION_RADIUS * playerSizeScale) : b.getCollisionRadius();
    float combinedRadius = radiusA + radiusB;

    return distSq < (combinedRadius * combinedRadius);
}

bool canEatFish(const Fish& player, const Fish& other) {
    float playerRadius = COLLISION_RADIUS * playerSizeScale;
    float otherRadius = other.getCollisionRadius();
    return playerRadius >= otherRadius * 0.95f;
}
////^^^^^^^^^^^

// ============================================
// تهيئة اللعبة
//شهد
// ============================================
void initGame() {
    glClearColor(0.07f, 0.01f, 0.75f, 1.0f);
    playerSizeScale = INITIAL_PLAYER_SIZE;
    prevMouseX = WINDOW_WIDTH / 2.0f;

    fishArray.clear();
    for (int i = 0; i < NUM_FISH; ++i) {
        fishArray.push_back(Fish());
    }

    glMatrixMode(GL_PROJECTION);
    gluOrtho2D(0.0, WINDOW_WIDTH, 0.0, WINDOW_HEIGHT);
}
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

// ============================================
// التحكم
//محمد سعيد كامل
// ============================================
void mouseMove(int x, int y) {
    float mouseX = static_cast<float>(x);
    float mouseY = static_cast<float>(y);
    player.move(mouseX, mouseY, prevMouseX);
    prevMouseX = mouseX;
}
//^^^^^^^^^^^^^^^^^^^^^^^
void keyboard(int key, int x, int y) {
    if (key == GLUT_KEY_F2) {
        isGameOver = false;
        gameTime = INITIAL_TIME;
        score = 0;
        allFishCollected = false;
        playerSizeScale = INITIAL_PLAYER_SIZE;
        prevMouseX = WINDOW_WIDTH / 2.0f;

        fishArray.clear();
        for (int i = 0; i < NUM_FISH; ++i) {
            fishArray.push_back(Fish());
        }
    }
    glutPostRedisplay();
}

// ============================================
// المؤقتات
// ساره
// ============================================
void animationTimer(int value) {
    glutPostRedisplay();
    glutTimerFunc(50, animationTimer, 0);
}

void gameTimer(int value) {
    if (!isGameOver && gameTime > 0) {
        --gameTime;
        if (gameTime == 0) {
            isGameOver = true;
        }
    }
    glutPostRedisplay();
    glutTimerFunc(1000, gameTimer, 0);
}
/// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

// ============================================
// العرض الرئيسي
// ============================================
void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    drawOcean();

    if (!isGameOver) {
        // خلفية UI
        glColor3f(0.0f, 0.0f, 0.0f);
        glBegin(GL_QUADS);
        glVertex2f(0, 560);
        glVertex2f(150, 560);
        glVertex2f(150, 600);
        glVertex2f(0, 600);
        glEnd();

        glBegin(GL_QUADS);
        glVertex2f(1050, 560);
        glVertex2f(1200, 560);
        glVertex2f(1200, 600);
        glVertex2f(1050, 600);
        glEnd();

        // النصوص
        glColor3f(1.0f, 1.0f, 1.0f);
        drawText("Score:", 10, 585);
        glColor3f(1.0f, 1.0f, 0.0f);
        drawNumber(60, 585, score);

        glColor3f(1.0f, 1.0f, 1.0f);
        drawText("Size:", 10, 570);
        glColor3f(0.3f, 1.0f, 0.8f);
        drawFloat(50, 570, playerSizeScale);

        glColor3f(1.0f, 1.0f, 1.0f);
        drawText("Time:", 1070, 575);
        glColor3f(0.3f, 1.0f, 0.3f);
        drawNumber(1120, 575, gameTime);

        // رسم اللاعب
        player.draw();

        // تحريك ورسم الأسماك
        float playerRadius = COLLISION_RADIUS * playerSizeScale;

        for (size_t i = 0; i < fishArray.size(); ++i) {
            fishArray[i].moveWithBehavior(player.x, player.y, playerRadius);

            if (checkCollision(player, fishArray[i])) {
                if (fishArray[i].isRedFish) {
                    if (!canEatFish(player, fishArray[i])) {
                        isGameOver = true;
                        break;
                    } else {
                        score += 3;
                        if (playerSizeScale < MAX_PLAYER_SIZE) {
                            playerSizeScale += GROWTH_INCREMENT * 2.0f;
                            if (playerSizeScale > MAX_PLAYER_SIZE)
                                playerSizeScale = MAX_PLAYER_SIZE;
                        }
                        fishArray.erase(fishArray.begin() + i);
                        --i;
                    }
                } else {
                    if (canEatFish(player, fishArray[i])) {
                        int points = fishArray[i].isLarge ? 2 : 1;
                        score += points;

                        if (playerSizeScale < MAX_PLAYER_SIZE) {
                            playerSizeScale += GROWTH_INCREMENT;
                            if (playerSizeScale > MAX_PLAYER_SIZE)
                                playerSizeScale = MAX_PLAYER_SIZE;
                        }

                        fishArray.erase(fishArray.begin() + i);
                        --i;
                    } else {
                        isGameOver = true;
                        break;
                    }
                }
            } else {
                fishArray[i].draw();
            }
        }

        // فحص الفوز
        if (fishArray.empty()) {
            allFishCollected = true;
            isGameOver = true;
        }

    } else {
        // شاشة نهاية اللعبة
        glColor3f(0.0f, 0.0f, 0.0f);
        glBegin(GL_QUADS);
        glVertex2f(200, 150);
        glVertex2f(1000, 150);
        glVertex2f(1000, 450);
        glVertex2f(200, 450);
        glEnd();

        if (allFishCollected) {
            glColor3f(0.3f, 1.0f, 0.3f);
            drawLargeText("YOU WIN!", 520, 400);
            glColor3f(1.0f, 1.0f, 0.5f);
            drawLargeText("Final Score:", 480, 280);
            glColor3f(1.0f, 1.0f, 0.0f);
            drawNumber(640, 280, score);
        } else {
            glColor3f(1.0f, 0.3f, 0.3f);
            drawLargeText("GAME OVER!", 500, 400);
            glColor3f(1.0f, 0.8f, 0.5f);
            drawLargeText("Score:", 520, 280);
            glColor3f(1.0f, 0.5f, 0.0f);
            drawNumber(610, 280, score);
        }

        glColor3f(0.8f, 0.8f, 0.8f);
        drawLargeText("Press F2 to Play Again", 450, 200);
    }

    glDisable(GL_BLEND);
    glutSwapBuffers();
}

// ============================================
// الدالة الرئيسية
// ============================================
int main(int argc, char *argv[]) {
    srand(static_cast<unsigned int>(time(nullptr)));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutInitWindowPosition(250, 200);
    glutCreateWindow("Fish Game - Clean Version");

    initGame();

    glutPassiveMotionFunc(mouseMove);
    glutSpecialFunc(keyboard);
    glutTimerFunc(0, animationTimer, 0);
    glutTimerFunc(0, gameTimer, 0);
    glutDisplayFunc(display);

    glutMainLoop();
    return 0;
}
