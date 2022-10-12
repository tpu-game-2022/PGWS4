#include "MyHeader.h"

// [2] チャレンジ問題
// 画面色をグラデーションさせるための関数
void Gradation(float* r, float* g, float* b)
{
	// 赤成分
	if (*b >= 1.0f) *r += 0.001f;
	if (*g >= 1.0f) *r -= 0.001f;
	if (*r > 1.0f) { *r = 1.0f; }
	else if(*r < 0.0f) { *r = 0.0f; }

	// 緑成分
	if (*r >= 1.0f) *g += 0.001f;
	if (*b >= 1.0f) *g -= 0.001f;
	if (*g > 1.0f) { *g = 1.0f; }
	else if (*g < 0.0f) { *g = 0.0f; }

	// 青成分
	if (*g >= 1.0f) *b += 0.001f;
	if (*r >= 1.0f) *b -= 0.001f;
	if (*b > 1.0f) { *b = 1.0f; }
	else if (*b < 0.0f) { *b = 0.0f; }
}