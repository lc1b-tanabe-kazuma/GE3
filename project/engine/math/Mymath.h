#pragma once
#include <cstdint>
#include <cmath>

namespace Math {

	// Vector4の構造体
	struct Vector4 {
		float x, y, z, w;
	};

	// Vector3の構造体
	struct Vector3 {
		float x, y, z;
	};

	// Vector2の構造体
	struct Vector2 {
		float x, y;
	};

	// 3x3の行列の構造体
	struct Matrix3x3 {
		float m[3][3];
	};

	// 4x4の行列の構造体
	struct Matrix4x4 {
		float m[4][4];
	};

	struct TransForm {
		Vector3 scale;
		Vector3 rotate;
		Vector3 translate;
	};

	inline Vector2 operator+(const Vector2& a, const Vector2& b) {
		return { a.x + b.x, a.y + b.y };
	}

	inline Vector2& operator+=(Vector2& a, const Vector2& b) {
		a.x += b.x;
		a.y += b.y;
		return a;
	}

	inline Vector2 operator-(const Vector2& a, const Vector2& b) {
		return { a.x - b.x, a.y - b.y };
	}

	inline Vector2 operator*(const Vector2& v, float s) {
		return { v.x * s, v.y * s };
	}

	inline Vector2 operator*(float s, const Vector2& v) {
		return { v.x * s, v.y * s };
	}

	inline Vector2& operator-=(Vector2& a, const Vector2& b) {
		a.x -= b.x;
		a.y -= b.y;
		return a;
	}

#pragma region 行列関連関数
	// 行列の積
	Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2);

	// 逆行列
	// 3x3行列式（余因子計算用）
	float Det3(float a1, float a2, float a3,
		float b1, float b2, float b3,
		float c1, float c2, float c3);

	Matrix4x4 Inverse(const Matrix4x4& m);

	// 平行移動行列
	Matrix4x4 MakeTransMatrix(const Vector3& v);

	//拡大縮小行列
	Matrix4x4 MakeScaleMatrix(const Vector3& v);

	// 座標変換
	Vector3 MakeTransform(const Matrix4x4& m, const Vector3& v);

	// 長さ
	float Length(const Vector3& v);

	// ベクトルの正規化
	Vector3 Normalize(const Vector3& v);

	// X軸の回転行列
	Matrix4x4 MakeRotXMatrix(float radian);

	// Y軸の回転行列
	Matrix4x4 MakeRotYMatrix(float radian);

	// Z軸の回転行列
	Matrix4x4 MakeRotZMatrix(float radian);

	// 単位行列を作る
	Matrix4x4 makeIdentity4x4();

	// 3次元アフィン変換行列
	Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate);

	// 透視投影行列
	Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip);

	// 正射影行列
	Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip);
#pragma endregion
};