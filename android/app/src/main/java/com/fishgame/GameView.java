package com.fishgame;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

public class GameView extends SurfaceView implements Runnable, SurfaceHolder.Callback {
    private static final int WINDOW_WIDTH = 1200;
    private static final int WINDOW_HEIGHT = 600;
    private static final int OCEAN_HEIGHT = 500;
    private static final int NUM_FISH = 20;
    private static final int INITIAL_TIME = 50;
    private static final float FISH_SIZE = 20f;
    private static final float COLLISION_RADIUS = 15f;
    private static final float FISH_SPEED = 4f; // pixels per frame adjusted for 60fps
    private static final float GROWTH_INCREMENT = 0.05f;
    private static final float INITIAL_PLAYER_SIZE = 1.0f;
    private static final float MAX_PLAYER_SIZE = 2.5f;
    private static final float FLEE_DISTANCE = 200f;
    private static final float CHASE_DISTANCE = 250f;
    private static final float FLEE_SPEED = 1.5f;
    private static final float CHASE_SPEED = 1.3f;

    private final SurfaceHolder holder;
    private Thread gameThread;
    private volatile boolean running = false;

    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Random random = new Random();

    private boolean isGameOver = false;
    private int score = 0;
    private int gameTime = INITIAL_TIME;
    private float playerSizeScale = INITIAL_PLAYER_SIZE;
    private float playerX = WINDOW_WIDTH / 2f;
    private float playerY = WINDOW_HEIGHT / 2f;
    private float prevTouchX = WINDOW_WIDTH / 2f;
    private boolean allFishCollected = false;

    private float touchX = playerX;
    private float touchY = playerY;

    private final List<Fish> fishArray = new ArrayList<>();

    public GameView(Context context) {
        super(context);
        holder = getHolder();
        holder.addCallback(this);
        initGame();
    }

    private void initGame() {
        isGameOver = false;
        allFishCollected = false;
        score = 0;
        gameTime = INITIAL_TIME;
        playerSizeScale = INITIAL_PLAYER_SIZE;
        prevTouchX = WINDOW_WIDTH / 2f;
        touchX = prevTouchX;
        touchY = WINDOW_HEIGHT / 2f;
        fishArray.clear();
        for (int i = 0; i < NUM_FISH; i++) {
            fishArray.add(new Fish(false));
        }
    }

    @Override
    public void run() {
        long lastTime = System.currentTimeMillis();
        long lastSecond = lastTime;
        while (running) {
            if (!holder.getSurface().isValid()) continue;

            long now = System.currentTimeMillis();
            long delta = now - lastTime;
            lastTime = now;

            update(delta / 16f); // approx scale to 60fps steps

            Canvas canvas = holder.lockCanvas();
            if (canvas != null) {
                drawGame(canvas);
                holder.unlockCanvasAndPost(canvas);
            }

            if (now - lastSecond >= 1000) {
                lastSecond = now;
                if (!isGameOver && gameTime > 0) {
                    gameTime--;
                    if (gameTime == 0) {
                        isGameOver = true;
                    }
                }
            }
        }
    }

    private void update(float stepScale) {
        if (isGameOver) return;

        // move player to touch
        float dxPlayer = touchX - playerX;
        float dyPlayer = touchY - playerY;
        playerX = touchX;
        playerY = touchY;
        float direction = (touchX > prevTouchX) ? FISH_SPEED : -FISH_SPEED;
        prevTouchX = touchX;

        float playerRadius = COLLISION_RADIUS * playerSizeScale;
        for (int i = 0; i < fishArray.size(); i++) {
            Fish f = fishArray.get(i);
            f.moveWithBehavior(playerX, playerY, playerRadius, stepScale);

            if (checkCollision(playerX, playerY, playerRadius, f)) {
                if (f.isRedFish) {
                    if (!canEatFish(playerRadius, f)) {
                        isGameOver = true;
                        break;
                    } else {
                        score += 3;
                        growPlayer(true);
                        fishArray.remove(i);
                        i--;
                    }
                } else {
                    if (canEatFish(playerRadius, f)) {
                        int points = f.isLarge ? 2 : 1;
                        score += points;
                        growPlayer(false);
                        fishArray.remove(i);
                        i--;
                    } else {
                        isGameOver = true;
                        break;
                    }
                }
            }
        }

        if (fishArray.isEmpty()) {
            allFishCollected = true;
            isGameOver = true;
        }
    }

    private void growPlayer(boolean extra) {
        float inc = extra ? GROWTH_INCREMENT * 2f : GROWTH_INCREMENT;
        playerSizeScale = Math.min(MAX_PLAYER_SIZE, playerSizeScale + inc);
    }

    private boolean checkCollision(float px, float py, float pr, Fish f) {
        float dx = px - f.x;
        float dy = py - f.y;
        float distSq = dx * dx + dy * dy;
        float combined = pr + f.getCollisionRadius();
        return distSq < combined * combined;
    }

    private boolean canEatFish(float playerRadius, Fish other) {
        float otherRadius = other.getCollisionRadius();
        return playerRadius >= otherRadius * 0.95f;
    }

    private void drawGame(Canvas canvas) {
        canvas.drawColor(Color.rgb(10, 60, 140));
        drawOcean(canvas);

        if (!isGameOver) {
            // UI background bars
            paint.setColor(Color.argb(180, 0, 0, 0));
            canvas.drawRect(0, WINDOW_HEIGHT - 40, 150, WINDOW_HEIGHT, paint);
            canvas.drawRect(WINDOW_WIDTH - 150, WINDOW_HEIGHT - 40, WINDOW_WIDTH, WINDOW_HEIGHT, paint);

            // texts
            paint.setColor(Color.WHITE);
            paint.setTextSize(22);
            canvas.drawText("Score:", 10, WINDOW_HEIGHT - 12, paint);
            paint.setColor(Color.YELLOW);
            canvas.drawText(String.valueOf(score), 70, WINDOW_HEIGHT - 12, paint);

            paint.setColor(Color.WHITE);
            paint.setTextSize(20);
            canvas.drawText("Size:", 10, WINDOW_HEIGHT - 30, paint);
            paint.setColor(Color.CYAN);
            canvas.drawText(String.format("%.2fx", playerSizeScale), 55, WINDOW_HEIGHT - 30, paint);

            paint.setColor(Color.WHITE);
            canvas.drawText("Time:", WINDOW_WIDTH - 120, WINDOW_HEIGHT - 20, paint);
            paint.setColor(Color.GREEN);
            canvas.drawText(String.valueOf(gameTime), WINDOW_WIDTH - 60, WINDOW_HEIGHT - 20, paint);

            // draw player
            drawFish(canvas, playerX, playerY, playerSizeScale, false, false, touchX >= prevTouchX);

            // draw fishes
            for (Fish f : fishArray) {
                drawFish(canvas, f.x, f.y, f.sizeScale, f.isRedFish, f.isLarge, f.direction > 0);
            }
        } else {
            paint.setColor(Color.BLACK);
            canvas.drawRect(200, 150, 1000, 450, paint);
            paint.setTextSize(60);
            if (allFishCollected) {
                paint.setColor(Color.GREEN);
                canvas.drawText("YOU WIN!", 480, 280, paint);
                paint.setColor(Color.YELLOW);
                paint.setTextSize(40);
                canvas.drawText("Final Score: " + score, 460, 340, paint);
            } else {
                paint.setColor(Color.RED);
                canvas.drawText("GAME OVER!", 440, 280, paint);
                paint.setColor(Color.rgb(255, 200, 120));
                paint.setTextSize(40);
                canvas.drawText("Score: " + score, 520, 340, paint);
            }
            paint.setColor(Color.LTGRAY);
            paint.setTextSize(32);
            canvas.drawText("Tap to Play Again", 480, 400, paint);
        }
    }

    private void drawOcean(Canvas canvas) {
        paint.setStyle(Paint.Style.FILL);
        // base gradient approximation
        paint.setColor(Color.rgb(13, 102, 204));
        canvas.drawRect(0, 0, WINDOW_WIDTH, OCEAN_HEIGHT, paint);

        // waves as paths
        paint.setColor(Color.argb(160, 50, 130, 220));
        paint.setStrokeWidth(3f);
        paint.setStyle(Paint.Style.STROKE);
        Path path = new Path();
        float waveOffset = (System.currentTimeMillis() % 10000L) / 1000f;
        int wavePoints = 40;
        path.reset();
        for (int i = 0; i <= wavePoints; i++) {
            float x = (WINDOW_WIDTH / (float) wavePoints) * i;
            float y = OCEAN_HEIGHT + (float) Math.sin((x * 0.01f) + waveOffset) * 8f;
            if (i == 0) path.moveTo(x, y);
            else path.lineTo(x, y);
        }
        canvas.drawPath(path, paint);
    }

    private void drawFish(Canvas canvas, float x, float y, float scale, boolean isRed, boolean isLarge, boolean facingRight) {
        float bodyLength = FISH_SIZE * 1.8f * scale;
        float bodyHeight = FISH_SIZE * 1.2f * scale;
        float dir = facingRight ? 1f : -1f;

        int bodyColor = isRed ? Color.rgb(255, 80, 80) : Color.rgb(255, 230, 50);
        paint.setStyle(Paint.Style.FILL);
        paint.setColor(bodyColor);

        // body (oval)
        canvas.save();
        canvas.translate(x, y);
        canvas.scale(dir, 1f);
        canvas.drawOval(-bodyLength * 0.5f, -bodyHeight * 0.5f, bodyLength * 0.5f, bodyHeight * 0.5f, paint);

        // tail
        Path tail = new Path();
        float tailBaseX = -bodyLength * 0.5f;
        float tailLength = FISH_SIZE * 0.6f * scale;
        tail.moveTo(tailBaseX, 0);
        tail.lineTo(tailBaseX - tailLength, tailLength * 0.6f);
        tail.lineTo(tailBaseX - tailLength, -tailLength * 0.6f);
        tail.close();
        paint.setColor(Color.rgb((int) (Color.red(bodyColor) * 0.3f), (int) (Color.green(bodyColor) * 0.8f), (int) (Color.blue(bodyColor) * 0.3f)));
        canvas.drawPath(tail, paint);

        // eye
        float eyeX = bodyLength * 0.35f;
        float eyeY = bodyHeight * 0.15f;
        float eyeSize = FISH_SIZE * 0.15f * scale;
        paint.setColor(Color.WHITE);
        canvas.drawCircle(eyeX, eyeY, eyeSize, paint);
        paint.setColor(Color.BLACK);
        canvas.drawCircle(eyeX, eyeY, eyeSize * 0.6f, paint);

        canvas.restore();
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (event.getAction() == MotionEvent.ACTION_DOWN || event.getAction() == MotionEvent.ACTION_MOVE) {
            touchX = event.getX();
            touchY = event.getY();
            if (isGameOver) {
                initGame();
            }
        }
        return true;
    }

    @Override
    public void surfaceCreated(SurfaceHolder surfaceHolder) {
        resume();
    }

    @Override
    public void surfaceChanged(SurfaceHolder surfaceHolder, int i, int i1, int i2) {
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder surfaceHolder) {
        pause();
    }

    public void pause() {
        running = false;
        if (gameThread != null) {
            try {
                gameThread.join();
            } catch (InterruptedException ignored) {
            }
        }
    }

    public void resume() {
        if (running) return;
        running = true;
        gameThread = new Thread(this);
        gameThread.start();
    }

    private class Fish {
        boolean isRedFish;
        boolean isLarge;
        float x, y;
        float direction;
        float sizeScale;

        Fish(boolean player) {
            isRedFish = random.nextInt(10) < 3;
            isLarge = random.nextInt(10) < 3;
            sizeScale = isLarge ? 1.5f : 0.9f;
            x = random.nextInt(WINDOW_WIDTH);
            y = random.nextInt(OCEAN_HEIGHT - 50) + 20;
            direction = random.nextBoolean() ? FISH_SPEED : -FISH_SPEED;
        }

        float getCollisionRadius() {
            return COLLISION_RADIUS * sizeScale;
        }

        void moveWithBehavior(float playerX, float playerY, float playerRadius, float stepScale) {
            float dx = playerX - x;
            float dy = playerY - y;
            float dist = (float) Math.sqrt(dx * dx + dy * dy);

            float stepX = direction * stepScale;
            float stepY = 0;

            if (playerRadius > getCollisionRadius() && dist < FLEE_DISTANCE) {
                stepX = -(dx / dist) * FISH_SPEED * FLEE_SPEED * stepScale;
                stepY = -(dy / dist) * FISH_SPEED * FLEE_SPEED * stepScale;
                direction = stepX < 0 ? -Math.abs(FISH_SPEED) : Math.abs(FISH_SPEED);
            } else if ((isRedFish || isLarge) && playerRadius < getCollisionRadius() && dist < CHASE_DISTANCE) {
                stepX = (dx / dist) * FISH_SPEED * CHASE_SPEED * stepScale;
                stepY = (dy / dist) * FISH_SPEED * CHASE_SPEED * stepScale;
                direction = stepX < 0 ? -Math.abs(FISH_SPEED) : Math.abs(FISH_SPEED);
            }

            x += stepX;
            y += stepY;

            if (x > WINDOW_WIDTH) x = 0;
            if (x < 0) x = WINDOW_WIDTH;
            if (y < 20) y = 20;
            if (y > OCEAN_HEIGHT - 20) y = OCEAN_HEIGHT - 20;
        }
    }
}

