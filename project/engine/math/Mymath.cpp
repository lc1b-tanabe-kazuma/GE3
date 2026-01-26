#include "Mymath.h"

using namespace std;

namespace Math {

#pragma region 行列関連関数
	// 行列の積
	Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2) {
		Matrix4x4 result = {};
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				result.m[i][j] = 0.0f;
				for (int k = 0; k < 4; k++) {
					result.m[i][j] += m1.m[i][k] * m2.m[k][j];
				}
			}
		}
		return result;
	};

	// 逆行列
	// 3x3行列式（余因子計算用）
	float Det3(float a1, float a2, float a3,
		float b1, float b2, float b3,
		float c1, float c2, float c3) {
		return a1 * (b2 * c3 - b3 * c2)
			- a2 * (b1 * c3 - b3 * c1)
			+ a3 * (b1 * c2 - b2 * c1);
	}

	Matrix4x4 Inverse(const Matrix4x4& m) {
		Matrix4x4 result = {};

		float det =
			m.m[0][0] * Det3(m.m[1][1], m.m[1][2], m.m[1][3], m.m[2][1], m.m[2][2], m.m[2][3], m.m[3][1], m.m[3][2], m.m[3][3]) -
			m.m[0][1] * Det3(m.m[1][0], m.m[1][2], m.m[1][3], m.m[2][0], m.m[2][2], m.m[2][3], m.m[3][0], m.m[3][2], m.m[3][3]) +
			m.m[0][2] * Det3(m.m[1][0], m.m[1][1], m.m[1][3], m.m[2][0], m.m[2][1], m.m[2][3], m.m[3][0], m.m[3][1], m.m[3][3]) -
			m.m[0][3] * Det3(m.m[1][0], m.m[1][1], m.m[1][2], m.m[2][0], m.m[2][1], m.m[2][2], m.m[3][0], m.m[3][1], m.m[3][2]);

		if (det == 0.0f) {
			// 行列が正則でない場合、単位行列または0行列などを返してもOK
			return result;
		}

		float invDet = 1.0f / det;

		// 余因子 + 転置で逆行列を一気に展開（各要素ベタ書き）
		result.m[0][0] = Det3(m.m[1][1], m.m[1][2], m.m[1][3], m.m[2][1], m.m[2][2], m.m[2][3], m.m[3][1], m.m[3][2], m.m[3][3]) * invDet;
		result.m[1][0] = -Det3(m.m[1][0], m.m[1][2], m.m[1][3], m.m[2][0], m.m[2][2], m.m[2][3], m.m[3][0], m.m[3][2], m.m[3][3]) * invDet;
		result.m[2][0] = Det3(m.m[1][0], m.m[1][1], m.m[1][3], m.m[2][0], m.m[2][1], m.m[2][3], m.m[3][0], m.m[3][1], m.m[3][3]) * invDet;
		result.m[3][0] = -Det3(m.m[1][0], m.m[1][1], m.m[1][2], m.m[2][0], m.m[2][1], m.m[2][2], m.m[3][0], m.m[3][1], m.m[3][2]) * invDet;

		result.m[0][1] = -Det3(m.m[0][1], m.m[0][2], m.m[0][3], m.m[2][1], m.m[2][2], m.m[2][3], m.m[3][1], m.m[3][2], m.m[3][3]) * invDet;
		result.m[1][1] = Det3(m.m[0][0], m.m[0][2], m.m[0][3], m.m[2][0], m.m[2][2], m.m[2][3], m.m[3][0], m.m[3][2], m.m[3][3]) * invDet;
		result.m[2][1] = -Det3(m.m[0][0], m.m[0][1], m.m[0][3], m.m[2][0], m.m[2][1], m.m[2][3], m.m[3][0], m.m[3][1], m.m[3][3]) * invDet;
		result.m[3][1] = Det3(m.m[0][0], m.m[0][1], m.m[0][2], m.m[2][0], m.m[2][1], m.m[2][2], m.m[3][0], m.m[3][1], m.m[3][2]) * invDet;

		result.m[0][2] = Det3(m.m[0][1], m.m[0][2], m.m[0][3], m.m[1][1], m.m[1][2], m.m[1][3], m.m[3][1], m.m[3][2], m.m[3][3]) * invDet;
		result.m[1][2] = -Det3(m.m[0][0], m.m[0][2], m.m[0][3], m.m[1][0], m.m[1][2], m.m[1][3], m.m[3][0], m.m[3][2], m.m[3][3]) * invDet;
		result.m[2][2] = Det3(m.m[0][0], m.m[0][1], m.m[0][3], m.m[1][0], m.m[1][1], m.m[1][3], m.m[3][0], m.m[3][1], m.m[3][3]) * invDet;
		result.m[3][2] = -Det3(m.m[0][0], m.m[0][1], m.m[0][2], m.m[1][0], m.m[1][1], m.m[1][2], m.m[3][0], m.m[3][1], m.m[3][2]) * invDet;

		result.m[0][3] = -Det3(m.m[0][1], m.m[0][2], m.m[0][3], m.m[1][1], m.m[1][2], m.m[1][3], m.m[2][1], m.m[2][2], m.m[2][3]) * invDet;
		result.m[1][3] = Det3(m.m[0][0], m.m[0][2], m.m[0][3], m.m[1][0], m.m[1][2], m.m[1][3], m.m[2][0], m.m[2][2], m.m[2][3]) * invDet;
		result.m[2][3] = -Det3(m.m[0][0], m.m[0][1], m.m[0][3], m.m[1][0], m.m[1][1], m.m[1][3], m.m[2][0], m.m[2][1], m.m[2][3]) * invDet;
		result.m[3][3] = Det3(m.m[0][0], m.m[0][1], m.m[0][2], m.m[1][0], m.m[1][1], m.m[1][2], m.m[2][0], m.m[2][1], m.m[2][2]) * invDet;

		return result;
	}

	// 平行移動行列
	Matrix4x4 MakeTransMatrix(const Vector3& v) {
		Matrix4x4 result = {};
		result.m[0][0] = 1.0f;
		result.m[1][1] = 1.0f;
		result.m[2][2] = 1.0f;
		result.m[3][3] = 1.0f;
		result.m[3][0] = v.x;
		result.m[3][1] = v.y;
		result.m[3][2] = v.z;
		return result;
	}

	//拡大縮小行列
	Matrix4x4 MakeScaleMatrix(const Vector3& v) {
		Matrix4x4 result = {};
		result.m[0][0] = v.x;
		result.m[1][1] = v.y;
		result.m[2][2] = v.z;
		result.m[3][3] = 1.0f;
		return result;
	}

	// 座標変換
	Vector3 MakeTransform(const Matrix4x4& m, const Vector3& v) {
		Vector3 result = {};
		result.x = m.m[0][0] * v.x + m.m[1][0] * v.y + m.m[2][0] * v.z + m.m[3][0];
		result.y = m.m[0][1] * v.x + m.m[1][1] * v.y + m.m[2][1] * v.z + m.m[3][1];
		result.z = m.m[0][2] * v.x + m.m[1][2] * v.y + m.m[2][2] * v.z + m.m[3][2];

		// w成分
		float w = v.x * m.m[0][3] + v.y * m.m[1][3] + v.z * m.m[2][3] + m.m[3][3];

		// wが0でない場合、結果をwで割る
		if (w != 0.0f) {
			result.x /= w;
			result.y /= w;
			result.z /= w;
		}
		return result;
	}

	// 長さ
	float Length(const Vector3& v) {

		float result = {};

		result = sqrtf((v.x * v.x) + (v.y * v.y) + (v.z * v.z));

		return result;
	};

	// ベクトルの正規化
	Vector3 Normalize(const Vector3& v) {

		Vector3 result = {};
		float length = Length(v);
		if (length != 0.0f) {
			result.x = v.x / Length(v);
			result.y = v.y / Length(v);
			result.z = v.z / Length(v);
		}
		return result;
	};

	// X軸の回転行列
	Matrix4x4 MakeRotXMatrix(float radian) {
		Matrix4x4 result = {};
		result.m[0][0] = 1.0f;
		result.m[1][1] = std::cosf(radian);
		result.m[1][2] = std::sinf(radian);
		result.m[2][1] = -std::sinf(radian);
		result.m[2][2] = std::cosf(radian);
		result.m[3][3] = 1.0f;
		return result;
	}

	// Y軸の回転行列
	Matrix4x4 MakeRotYMatrix(float radian) {
		Matrix4x4 result = {};
		result.m[0][0] = std::cosf(radian);
		result.m[0][2] = -std::sinf(radian);
		result.m[1][1] = 1.0f;
		result.m[2][0] = std::sinf(radian);
		result.m[2][2] = std::cosf(radian);
		result.m[3][3] = 1.0f;
		return result;
	}

	// Z軸の回転行列
	Matrix4x4 MakeRotZMatrix(float radian) {
		Matrix4x4 result = {};
		result.m[0][0] = std::cosf(radian);
		result.m[0][1] = std::sinf(radian);
		result.m[1][0] = -std::sinf(radian);
		result.m[1][1] = std::cosf(radian);
		result.m[2][2] = 1.0f;
		result.m[3][3] = 1.0f;
		return result;
	}

	// 単位行列を作る
	Matrix4x4 makeIdentity4x4() {
		Matrix4x4 result = {};
		result.m[0][0] = 1.0f;
		result.m[1][1] = 1.0f;
		result.m[2][2] = 1.0f;
		result.m[3][3] = 1.0f;
		return result;
	}

	// 3次元アフィン変換行列
	Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate) {

		// スケーリング行列の作成
		Matrix4x4 matScale = MakeScaleMatrix(scale);

		Matrix4x4 matRotX = MakeRotXMatrix(rotate.x);
		Matrix4x4 matRotY = MakeRotYMatrix(rotate.y);
		Matrix4x4 matRotZ = MakeRotZMatrix(rotate.z);

		// 回転行列の合成
		Matrix4x4 matRot = Multiply(Multiply(matRotY, matRotX), matRotZ);

		// 平行移動行列の作成
		Matrix4x4 matTrans = MakeTransMatrix(translate);

		// スケーリング、回転、平行移動の合成
		Matrix4x4 matTransform = Multiply(Multiply(matScale, matRot), matTrans);

		return matTransform;
	}

	// 透視投影行列
	Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip) {
		Matrix4x4 result = {};
		float f = 1.0f / std::tanf(fovY / 2.0f);
		result.m[0][0] = (f * (1.0f / aspectRatio));
		result.m[1][1] = f;
		result.m[2][2] = (farClip) / (farClip - nearClip);
		result.m[2][3] = 1.0f;
		result.m[3][2] = -nearClip * farClip / (farClip - nearClip);
		return result;
	}

	// 正射影行列
	Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip) {
		Matrix4x4 result = {};
		result.m[0][0] = 2.0f / (right - left);
		result.m[1][1] = 2.0f / (top - bottom);
		result.m[2][2] = 1.0f / (farClip - nearClip);
		result.m[3][0] = (right + left) / (left - right);
		result.m[3][1] = (top + bottom) / (bottom - top);
		result.m[3][2] = (nearClip) / (nearClip - farClip);
		result.m[3][3] = 1.0f;
		return result;
	}
#pragma endregion
}