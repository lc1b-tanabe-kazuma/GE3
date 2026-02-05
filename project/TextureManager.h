#pragma once
class TextureManager {
public:
	// シングルトンインスタンスの取得
	static TextureManager* GetInstance();

	// 終了
	static void Finalize();

private:
	static TextureManager* instance;

	TextureManager() = default;
	~TextureManager() = default;
	TextureManager(TextureManager&) = delete;
	TextureManager& operator=(const TextureManager&) = delete;
};