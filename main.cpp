#include <Novice.h>
#include <vector>
#include <cmath>
#include <algorithm> // for std::remove_if

const char kWindowTitle[] = "﻿魔法結界を守れ！";

// 2Dベクトル構造体
struct Vector2 {
	float x; // X座標
	float y; // Y座標
};

// プレイヤー情報
struct Player {
	Vector2 pos;// 位置
	float radius;// 当たり判定に使う円の半径
	int speed;// 移動速度
	int shotgunLevel;// ショットガンのレベル（弾の拡散範囲に影響）
	int shotgunTimer;// ショットガン効果の残り時間（フレーム単位）
};

// 弾の情報
struct Bullet {
	Vector2 pos;// 位置
	float radius;// 当たり判定に使う円の半径
	int speed;// 移動速度
	Vector2 direction;// 移動方向（正規化されたベクトル）
	bool isAlive;// 生存フラグ（trueなら画面内に存在）
};

// 敵の情報
struct Enemy {
	Vector2 pos;// 位置
	float radius;// 当たり判定に使う円の半径
	int speed;// 移動速度
	bool isAlive;// 生存フラグ
	int hp;// 体力
};

// 連射速度アップアイテム
struct PowerUp {
	Vector2 pos;
	float radius;
	int speed;
	bool isAlive;
	int hp;// 壊すための体力
};

// 弾数増加（ショットガン化）アイテム
struct ShotgunPowerUp {
	Vector2 pos;
	float radius;
	int speed;
	bool isAlive;
	int hp;// 壊すための体力
};

// タル爆弾
struct Bakudan {
	Vector2 pos;
	float radius;
	int speed;
	bool isAlive;
	int hp;// 壊すための体力
};


// シーン管理（ゲームの状態）
enum Scene {
	TITLE,// タイトル画面
	EXPLANATION1,// 説明画面
	EXPLANATION2,//ステージ２説明
	EXPLANATION3,//ステージ３説明
	SELECTION,// ステージ選択画面
	GAME1,// ゲーム本編（ステージ1）
	GAME2,// ゲーム本編（ステージ2）
	GAME3,// ゲーム本編（ステージ3）
	CLEAR,// クリア画面
	OVER,// ゲームオーバー画面
};
int scene = TITLE;

// 2つのオブジェクトが近すぎるかどうかを判定する関数
bool IsTooClose(const Vector2& pos1, float radius1, const Vector2& pos2, float radius2) {
	float dx = pos1.x - pos2.x;
	float dy = pos1.y - pos2.y;
	float distance = sqrtf(dx * dx + dy * dy);
	// 半径の合計 + 余白50以内なら「近すぎる」と判定
	return distance < (radius1 + radius2 + 50.0f);
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	// ウィンドウサイズ
	const int kWindowWidth = 1280;
	const int kWindowHeight = 720;

	// Noviceライブラリ初期化
	Novice::Initialize(kWindowTitle, kWindowWidth, kWindowHeight);

	// キー入力管理
	char keys[256] = { 0 };
	char preKeys[256] = { 0 };

	// プレイヤー初期化（画面下中央に配置）
	Player player = { {kWindowWidth / 2.0f, kWindowHeight - 100.0f}, 48.0f, 8, 0, 0 };
	int playerHandle = Novice::LoadTexture("./Resources/player.png");

	// 弾管理
	std::vector<Bullet> bullets;    // 発射された弾リスト
	int shotCooldown = 0;           // 次の弾が撃てるまでの待ち時間
	int defaultShotCooldown = 15;   // 連射速度の基準値
	int powerUpLevel = 0;           // 連射速度強化のレベル
	int mahoudanHandle = Novice::LoadTexture("./Resources/mahoudan.png"); // 魔法弾画像

	//ゴブリン管理
	std::vector<Enemy> goburins;
	int enemySpawnTimer = 0;        // 敵出現までのタイマー
	int lives = 20;                 // ライフ（防衛ラインに侵入されると減る）
	int goburinHandle = Novice::LoadTexture("./Resources/goburin.png");

	//コウモリ管理
	std::vector<Enemy> komoris;
	int komoriHandle = Novice::LoadTexture("./Resources/komori.png");

	// アイテム管理
	std::vector<PowerUp> powerUps;         // 連射速度アップ
	std::vector<ShotgunPowerUp> shotgunPowerUps; // ショットガン化
	int powerUpSpawnTimer = 0;      // 連射速度アイテム出現までのタイマー
	int shotgunPowerUpSpawnTimer = 0; // ショットガンアイテム出現までのタイマー
	int itemHandle = Novice::LoadTexture("./Resources/item.png");

	////タル爆弾管理
	std::vector<Bakudan>bakudans;
	int bakudanTimer = 0;//爆弾出現までのタイマー
	int bakudanHandle = Novice::LoadTexture("./Resources/bakudan.png");


	//タイマー 数字画像をロード
	int numberGrahs[10] = {};
	for (int i = 0; i < 10; i++) {
		char filePath[64];
		snprintf(filePath, sizeof(filePath), "./Resources/%d.png", i); // 安全な関数
		numberGrahs[i] = Novice::LoadTexture(filePath);
	}
	const int graphWidth = 71;
	int frame = 0;   // フレームカウント
	int seconds = 0; // 秒数


	// サウンドデータ
	int Bgmtitle = Novice::LoadAudio("./Resources/Sounds/title.mp3");//タイトル音
	int Bgmketei = Novice::LoadAudio("./Resources/Sounds/ketei.mp3");//決定音
	int Bgmcancel = Novice::LoadAudio("./Resources/Sounds/cancel.mp3");//戻る音
	int Bgmmahou = Novice::LoadAudio("./Resources/Sounds/mahou.mp3");//魔法音
	int Bgmstage1 = Novice::LoadAudio("./Resources/Sounds/stage1.mp3");//ステージ１音
	

	//サウンドハンドル
	int titleBGM = -1;// タイトルBGM
	int keteiSE = -1;//決定SE
	int canselSE = -1;//戻るSE
	int mahouSE = -1;//魔法SE

	int Stage1BGM = -1;//ステージ１BGM






	//HPマーク
	int hatoHandle = Novice::LoadTexture("./Resources/ha-to.png");
	//魔法陣耐久値マーク
	int tateHandle = Novice::LoadTexture("./Resources/tate.png");

	//タイトル画面
	int tilteHandle = Novice::LoadTexture("./Resources/Tilte.png");
	//説明1
	int Explanation1 = Novice::LoadTexture("./Resources/Setumei.png");
	//説明2
	int Explanation2 = Novice::LoadTexture("./Resources/Setumei2.png");
	//説明3
	int Explanation3 = Novice::LoadTexture("./Resources/Setumei3.png");
	//ステージ選択
	int selectHandle = Novice::LoadTexture("./Resources/SELECT.png");
	//ステージ
	int stageHandle = Novice::LoadTexture("./Resources/stage.png");
	//ゲームクリア
	int clearHandle = Novice::LoadTexture("./Resources/CLEA.png");
	//ゲームオーバ
	int overHandle = Novice::LoadTexture("./Resources/GAMEOVER.png");

	// プレイヤー移動範囲（左右の壁）
	int minX = 360;
	int maxX = 920;
	int range = maxX - minX;

	// アイテムの出現範囲
	int minItemX = minX + 100;
	int maxItemX = maxX - 100;
	int itemRange = maxItemX - minItemX;

	// ゲーム制限時間
	int gameTimer = 0;              // 経過フレーム
	const int framePerSecond = 60; // FPS
	const int totalTime = 60 * framePerSecond; // 60秒（クリア条件）

	// ゲーム初期化処理を関数として定義
	auto initGame = [&]() {
		bullets.clear();
		goburins.clear();
		komoris.clear();
		lives = 20;
		player.pos = { kWindowWidth / 2.0f, kWindowHeight - 100.0f };
		gameTimer = 0;
		shotCooldown = 0;
		powerUpSpawnTimer = 0;
		shotgunPowerUpSpawnTimer = 0;
		powerUps.clear();
		shotgunPowerUps.clear();
		bakudans.clear();
		bakudanTimer = 0;
		defaultShotCooldown = 15;
		powerUpLevel = 0;
		player.shotgunLevel = 0;
		player.shotgunTimer = 0;
		frame = 0; // タイマーをリセット
		seconds = 0;
		};
	initGame(); // 初回起動時に初期化

	// メインループ
	while (Novice::ProcessMessage() == 0) {
		Novice::BeginFrame();

		// キー入力更新
		memcpy(preKeys, keys, 256);
		Novice::GetHitKeyStateAll(keys);

		/// ====================
		/// 更新処理
		/// ====================
		switch (scene) {
		case TITLE: // タイトル画面
			// サウンドが再生されていなければ再生開始
			if (!Novice::IsPlayingAudio(titleBGM)) {
				titleBGM = Novice::PlayAudio(Bgmtitle, false, 1.0f);
			}
			if (preKeys[DIK_RETURN] == 0 && keys[DIK_RETURN] != 0) {
				scene = EXPLANATION1; //説明画面へ
				keteiSE = Novice::PlayAudio(Bgmketei, false, 1.0f);//決定音
			}
			break;

		case EXPLANATION1:// 説明1画面
			if (preKeys[DIK_RETURN] == 0 && keys[DIK_RETURN] != 0) {
				scene = SELECTION; //ステージ選択へ
				keteiSE = Novice::PlayAudio(Bgmketei, false, 1.0f);//決定音
			}
			if (preKeys[DIK_2] == 0 && keys[DIK_2] != 0) {
				scene = EXPLANATION2; //説明2画面へ
				keteiSE = Novice::PlayAudio(Bgmketei, false, 1.0f);//決定音
			}
			if (preKeys[DIK_3] == 0 && keys[DIK_3] != 0) {
				scene = EXPLANATION3; // 説明3画面へ
				keteiSE = Novice::PlayAudio(Bgmketei, false, 1.0f);//決定音
			}

			break;
		case EXPLANATION2:// 説明2画面
			if (preKeys[DIK_RETURN] == 0 && keys[DIK_RETURN] != 0) {
				scene = SELECTION;//ステージ選択へ
			}
			if (preKeys[DIK_1] == 0 && keys[DIK_1] != 0) {
				scene = EXPLANATION1; //説明1画面へ
			}
			if (preKeys[DIK_3] == 0 && keys[DIK_3] != 0) {
				scene = EXPLANATION3; // 説明3画面へ
			}
			break;
		case EXPLANATION3:// 説明3画面
			if (preKeys[DIK_RETURN] == 0 && keys[DIK_RETURN] != 0) {
				scene = SELECTION; //ステージ選択へ
			}
			if (preKeys[DIK_1] == 0 && keys[DIK_1] != 0) {
				scene = EXPLANATION1; //説明1画面へ
			}
			if (preKeys[DIK_2] == 0 && keys[DIK_2] != 0) {
				scene = EXPLANATION2; // 説明2画面へ
			}

			break;
		case SELECTION: // 選択画面
			if (preKeys[DIK_1] == 0 && keys[DIK_1] != 0) {
				scene = GAME1;
				Novice::StopAudio(titleBGM); // 再生中の音を止める
				initGame(); // ゲーム開始時に毎回初期化
			}
			if (preKeys[DIK_2] == 0 && keys[DIK_2] != 0) {
				scene = GAME2;
				Novice::StopAudio(titleBGM); // 再生中の音を止める
				initGame(); // ゲーム開始時に毎回初期化
			}
			if (preKeys[DIK_3] == 0 && keys[DIK_3] != 0) {
				scene = GAME3;
				Novice::StopAudio(titleBGM); // 再生中の音を止める
				initGame(); // ゲーム開始時に毎回初期化
			}
			if (preKeys[DIK_BACKSPACE] == 0 && keys[DIK_BACKSPACE] != 0) {
				scene = EXPLANATION1;
				initGame(); // ゲーム開始時に毎回初期化
				canselSE = Novice::PlayAudio(Bgmcancel, false, 1.0f);//戻る音
			}


			break;

		case GAME1: {//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
			// サウンドが再生されていなければ再生開始
			if (!Novice::IsPlayingAudio(Stage1BGM)) {
				Stage1BGM = Novice::PlayAudio(Bgmstage1, false, 1.0f);
			}


			//リソースタイマーの更新
			frame++;
			seconds = frame / 60; // ここの値を変更
			if (seconds >= 60) {
				scene = CLEAR;
				Novice::StopAudio(Stage1BGM); // 再生中の音を止める
			}
			//---------------------------------------------------

			// ゲーム時間経過
			gameTimer++;
			if (gameTimer >= totalTime) {
				scene = CLEAR; // 制限時間を耐えればクリア
			}

			// プレイヤー操作（AとDキーで左右移動）
			if (keys[DIK_A]) { player.pos.x -= player.speed; }
			if (keys[DIK_D]) { player.pos.x += player.speed; }

			// プレイヤーの移動制限（画面外に出ないように）
			float minPlayerX = minX + player.radius;
			float maxPlayerX = maxX - player.radius;
			if (player.pos.x < minPlayerX) player.pos.x = minPlayerX;
			if (player.pos.x > maxPlayerX) player.pos.x = maxPlayerX;

			// 弾発射（スペースキー）
			if (keys[DIK_SPACE] && shotCooldown == 0) {
				
				if (player.shotgunLevel == 0) {
					// 通常弾
					bullets.push_back({ {player.pos.x + 16, player.pos.y}, 16.0f, 15, {0.0f, -1.0f}, true });
				} else {
					// ショットガン（複数方向に発射）
					float angleIncrement = 0.15f;
					for (int i = -player.shotgunLevel; i <= player.shotgunLevel; ++i) {
						float angle = angleIncrement * i;
						bullets.push_back({ {player.pos.x + 16, player.pos.y}, 16.0f, 15, {sinf(angle), -cosf(angle)}, true });
					}
				}
				// クールダウン設定（連射速度に影響）
				shotCooldown = defaultShotCooldown;
				mahouSE = Novice::PlayAudio(Bgmmahou, false, 1.0f);//魔法音
			}
			if (shotCooldown > 0) shotCooldown--;

			// 弾の移動処理
			for (auto& b : bullets) {
				if (b.isAlive) {
					b.pos.x += b.direction.x * b.speed;
					b.pos.y += b.direction.y * b.speed;
					if (b.pos.y < 0) b.isAlive = false; // 画面外に出たら消える
				}
			}

			// 敵の出現処理
			enemySpawnTimer++;
			if (enemySpawnTimer > 60) { // 1秒ごとに出現
				enemySpawnTimer = 0;
				float enemyRadius = 32.0f;
				// 出現位置をランダムに決定
				int safeMinX = minX + (int)player.radius;
				int safeRange = range - (int)(player.radius * 2);

				// 経過時間によって敵の体力を上げる
				int timeInSeconds = gameTimer / framePerSecond;
				int enemyInitialHP = (timeInSeconds <= 10) ? 1 : 1 + (timeInSeconds / 10) * 5;

				Enemy e = { {(float)(safeMinX + rand() % safeRange), 0.0f}, enemyRadius, 2, true, enemyInitialHP };
				goburins.push_back(e);
			}

			// 敵の移動処理
			for (auto& e : goburins) {
				if (e.isAlive) {
					e.pos.y += e.speed;
					// 防衛ラインを超えたらライフ減少
					if (e.pos.y > kWindowHeight - 200) {
						e.isAlive = false;
						lives--;
						if (lives <= 0) scene = OVER;
					}
				}
			}

			// アイテムの移動処理と非アクティブ化
			for (auto& p : powerUps) {
				if (p.isAlive) {
					p.pos.y += p.speed;
					// 画面下端に出たら非アクティブ化
					if (p.pos.y > kWindowHeight) {
						p.isAlive = false;
					}
				}
			}
			for (auto& s : shotgunPowerUps) {
				if (s.isAlive) {
					s.pos.y += s.speed;
					// 画面下端に出たら非アクティブ化
					if (s.pos.y > kWindowHeight) {
						s.isAlive = false;
					}
				}
			}

			// アイテム出現処理（連射速度アップ）
			powerUpSpawnTimer++;
			if (powerUpSpawnTimer >= 5 * framePerSecond) {
				PowerUp p = { {0.0f, 0.0f}, 50.0f, 3, true, 3 };
				bool spawnable = false;
				int maxAttempts = 50;
				for (int i = 0; i < maxAttempts; ++i) {
					float newX = (float)(minItemX + rand() % itemRange);
					p.pos = { newX, 0.0f };
					spawnable = true;
					for (const auto& existingP : powerUps) {
						if (IsTooClose(p.pos, p.radius, existingP.pos, existingP.radius)) {
							spawnable = false;
							break;
						}
					}
					for (const auto& existingS : shotgunPowerUps) {
						if (IsTooClose(p.pos, p.radius, existingS.pos, existingS.radius)) {
							spawnable = false;
							break;
						}
					}
					for (const auto& existingE : goburins) {
						if (IsTooClose(p.pos, p.radius, existingE.pos, existingE.radius)) {
							spawnable = false;
							break;
						}
					}
					if (spawnable) break;
				}
				if (spawnable) powerUps.push_back(p);
				// タイマーをゼロにリセット
				powerUpSpawnTimer = 0;
			}

			// アイテム出現処理（ショットガン化）
			shotgunPowerUpSpawnTimer++;
			if (shotgunPowerUpSpawnTimer >= 10 * framePerSecond) {
				ShotgunPowerUp s = { {0.0f, 0.0f}, 50.0f, 3, true, 5 };
				bool spawnable = false;
				int maxAttempts = 50;
				for (int i = 0; i < maxAttempts; ++i) {
					float newX = (float)(minItemX + rand() % itemRange);
					s.pos = { newX, 0.0f };
					spawnable = true;
					for (const auto& existingP : powerUps) {
						if (IsTooClose(s.pos, s.radius, existingP.pos, existingP.radius)) {
							spawnable = false;
							break;
						}
					}
					for (const auto& existingS : shotgunPowerUps) {
						if (IsTooClose(s.pos, s.radius, existingS.pos, existingS.radius)) {
							spawnable = false;
							break;
						}
					}
					for (const auto& existingE : goburins) {
						if (IsTooClose(s.pos, s.radius, existingE.pos, existingE.radius)) {
							spawnable = false;
							break;
						}
					}
					if (spawnable) break;
				}
				if (spawnable) shotgunPowerUps.push_back(s);
				// タイマーをゼロにリセット
				shotgunPowerUpSpawnTimer = 0;
			}

			// 弾と敵・アイテムの当たり判定
			for (auto& b : bullets) {
				if (!b.isAlive) continue;

				// 敵との衝突
				for (auto& e : goburins) {
					if (!e.isAlive) continue;
					float dx = b.pos.x - e.pos.x;
					float dy = b.pos.y - e.pos.y;
					float dist = sqrtf(dx * dx + dy * dy);
					if (dist < b.radius + e.radius) {
						b.isAlive = false;
						e.hp--;
						if (e.hp <= 0) e.isAlive = false;
					}
				}

				// 連射速度アップアイテムとの衝突
				for (auto& p : powerUps) {
					if (!p.isAlive) continue;
					float dx = b.pos.x - p.pos.x;
					float dy = b.pos.y - p.pos.y;
					float dist = sqrtf(dx * dx + dy * dy);
					if (dist < b.radius + p.radius) {
						b.isAlive = false;
						p.hp--;
						if (p.hp <= 0) {
							p.isAlive = false;
							// レベルに応じて連射速度を短縮
							powerUpLevel++;
							if (powerUpLevel == 1) defaultShotCooldown = 10;
							else if (powerUpLevel == 2) defaultShotCooldown = 8;
							else if (powerUpLevel == 3) defaultShotCooldown = 6;
							else defaultShotCooldown = 4;
						}
					}
				}

				// ショットガンアイテムとの衝突
				for (auto& s : shotgunPowerUps) {
					if (!s.isAlive) continue;
					float dx = b.pos.x - s.pos.x;
					float dy = b.pos.y - s.pos.y;
					float dist = sqrtf(dx * dx + dy * dy);
					if (dist < b.radius + s.radius) {
						b.isAlive = false;
						s.hp--;
						if (s.hp <= 0) {
							s.isAlive = false;
							player.shotgunLevel++;
							player.shotgunTimer = 6 * framePerSecond; // 効果6秒間
						}
					}
				}
			}

			// ショットガン効果時間を減らす
			if (player.shotgunTimer > 0) {
				player.shotgunTimer--;
				if (player.shotgunTimer <= 0) {
					player.shotgunLevel = 0; // 効果終了
				}
			}

			// 不要になったオブジェクトをvectorから削除（ガベージコレクション）
			bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](const Bullet& b) {
				return !b.isAlive;
				}), bullets.end());
			goburins.erase(std::remove_if(goburins.begin(), goburins.end(), [](const Enemy& e) {
				return !e.isAlive;
				}), goburins.end());
			powerUps.erase(std::remove_if(powerUps.begin(), powerUps.end(), [](const PowerUp& p) {
				return !p.isAlive;
				}), powerUps.end());
			shotgunPowerUps.erase(std::remove_if(shotgunPowerUps.begin(), shotgunPowerUps.end(), [](const ShotgunPowerUp& s) {
				return !s.isAlive;
				}), shotgunPowerUps.end());

		} break;

		case GAME2: {//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
			//リソースタイマーの更新
			frame++;
			seconds = frame / 60; // ここの値を変更
			if (seconds >= 60) {
				scene = CLEAR;
			}
			//---------------------------------------------------


			// ゲーム時間経過
			gameTimer++;
			if (gameTimer >= totalTime) {
				scene = CLEAR; // 制限時間を耐えればクリア
			}

			// プレイヤー操作（AとDキーで左右移動）
			if (keys[DIK_A]) { player.pos.x -= player.speed; }
			if (keys[DIK_D]) { player.pos.x += player.speed; }

			// プレイヤーの移動制限（画面外に出ないように）
			float minPlayerX = minX + player.radius;
			float maxPlayerX = maxX - player.radius;
			if (player.pos.x < minPlayerX) player.pos.x = minPlayerX;
			if (player.pos.x > maxPlayerX) player.pos.x = maxPlayerX;

			// 弾発射（スペースキー）
			if (keys[DIK_SPACE] && shotCooldown == 0) {
				if (player.shotgunLevel == 0) {
					// 通常弾
					bullets.push_back({ {player.pos.x + 16, player.pos.y}, 16.0f, 15, {0.0f, -1.0f}, true });
				} else {
					// ショットガン（複数方向に発射）
					float angleIncrement = 0.15f;
					for (int i = -player.shotgunLevel; i <= player.shotgunLevel; ++i) {
						float angle = angleIncrement * i;
						bullets.push_back({ {player.pos.x + 16, player.pos.y}, 16.0f, 15, {sinf(angle), -cosf(angle)}, true });
					}
				}
				// クールダウン設定（連射速度に影響）
				shotCooldown = defaultShotCooldown;
			}
			if (shotCooldown > 0) shotCooldown--;

			// 弾の移動処理
			for (auto& b : bullets) {
				if (b.isAlive) {
					b.pos.x += b.direction.x * b.speed;
					b.pos.y += b.direction.y * b.speed;
					if (b.pos.y < 0) b.isAlive = false; // 画面外に出たら消える
				}
			}



			// 敵の出現処理
			enemySpawnTimer++;
			if (enemySpawnTimer > 60) { // 1秒ごとに出現
				enemySpawnTimer = 0;
				float enemyRadius = 32.0f;
				// 出現位置をランダムに決定
				int safeMinX = minX + (int)player.radius;
				int safeRange = range - (int)(player.radius * 2);

				// 経過時間によって敵の体力を上げる
				int timeInSeconds = gameTimer / framePerSecond;
				int enemyInitialHP = (timeInSeconds <= 10) ? 1 : 1 + (timeInSeconds / 10) * 5;

				Enemy e = { {(float)(safeMinX + rand() % safeRange), 0.0f}, enemyRadius, 2, true, enemyInitialHP };
				komoris.push_back(e);
			}
			// 敵の移動処理
			for (auto& e : komoris) {
				if (e.isAlive) {
					e.pos.y += e.speed;
					// 防衛ラインを超えたらライフ減少
					if (e.pos.y > kWindowHeight - 200) {
						e.isAlive = false;
						lives--;
						if (lives <= 0) scene = OVER;
					}
				}
			}

			// アイテムの移動処理と非アクティブ化
			for (auto& p : powerUps) {
				if (p.isAlive) {
					p.pos.y += p.speed;
					// 画面下端に出たら非アクティブ化
					if (p.pos.y > kWindowHeight) {
						p.isAlive = false;
					}
				}
			}
			for (auto& s : shotgunPowerUps) {
				if (s.isAlive) {
					s.pos.y += s.speed;
					// 画面下端に出たら非アクティブ化
					if (s.pos.y > kWindowHeight) {
						s.isAlive = false;
					}
				}
			}

			//// 爆弾の移動処理と非アクティブ化
			for (auto& b : bakudans) {
				if (b.isAlive) {
					b.pos.y += b.speed;
					// 画面下端に出たら非アクティブ化
					if (b.pos.y > kWindowHeight) {
						b.isAlive = false;
					}
				}
			}

			// アイテム出現処理（連射速度アップ）
			powerUpSpawnTimer++;
			if (powerUpSpawnTimer >= 5 * framePerSecond) {
				PowerUp p = { {0.0f, 0.0f}, 50.0f, 3, true, 3 };
				bool spawnable = false;
				int maxAttempts = 50;
				for (int i = 0; i < maxAttempts; ++i) {
					float newX = (float)(minItemX + rand() % itemRange);
					p.pos = { newX, 0.0f };
					spawnable = true;
					for (const auto& existingP : powerUps) {
						if (IsTooClose(p.pos, p.radius, existingP.pos, existingP.radius)) {
							spawnable = false;
							break;
						}
					}
					for (const auto& existingS : shotgunPowerUps) {
						if (IsTooClose(p.pos, p.radius, existingS.pos, existingS.radius)) {
							spawnable = false;
							break;
						}
					}
					for (const auto& existingE : goburins) {
						if (IsTooClose(p.pos, p.radius, existingE.pos, existingE.radius)) {
							spawnable = false;
							break;
						}
					}
					if (spawnable) break;
				}
				if (spawnable) powerUps.push_back(p);
				// タイマーをゼロにリセット
				powerUpSpawnTimer = 0;
			}

			// アイテム出現処理（ショットガン化）
			shotgunPowerUpSpawnTimer++;
			if (shotgunPowerUpSpawnTimer >= 10 * framePerSecond) {
				ShotgunPowerUp s = { {0.0f, 0.0f}, 50.0f, 3, true, 5 };
				bool spawnable = false;
				int maxAttempts = 50;
				for (int i = 0; i < maxAttempts; ++i) {
					float newX = (float)(minItemX + rand() % itemRange);
					s.pos = { newX, 0.0f };
					spawnable = true;
					for (const auto& existingP : powerUps) {
						if (IsTooClose(s.pos, s.radius, existingP.pos, existingP.radius)) {
							spawnable = false;
							break;
						}
					}
					for (const auto& existingS : shotgunPowerUps) {
						if (IsTooClose(s.pos, s.radius, existingS.pos, existingS.radius)) {
							spawnable = false;
							break;
						}
					}
					for (const auto& existingE : goburins) {
						if (IsTooClose(s.pos, s.radius, existingE.pos, existingE.radius)) {
							spawnable = false;
							break;
						}
					}
					if (spawnable) break;
				}
				if (spawnable) shotgunPowerUps.push_back(s);
				// タイマーをゼロにリセット
				shotgunPowerUpSpawnTimer = 0;
			}

			// 爆弾出現処理
			bakudanTimer++;
			if (bakudanTimer >= 5 * framePerSecond) {
				Bakudan b = { {0.0f, 0.0f}, 50.0f, 3, true, 3 };
				bool spawnable = false;
				int maxAttempts = 50;
				for (int i = 0; i < maxAttempts; ++i) {
					float newX = (float)(minItemX + rand() % itemRange);
					b.pos = { newX, 0.0f };
					spawnable = true;
					for (const auto& existingP : powerUps) {
						if (IsTooClose(b.pos, b.radius, existingP.pos, existingP.radius)) {
							spawnable = false;
							break;
						}
					}
					for (const auto& existingS : shotgunPowerUps) {
						if (IsTooClose(b.pos, b.radius, existingS.pos, existingS.radius)) {
							spawnable = false;
							break;
						}
					}
					for (const auto& existingE : goburins) {
						if (IsTooClose(b.pos, b.radius, existingE.pos, existingE.radius)) {
							spawnable = false;
							break;
						}
					}
					if (spawnable) break;
				}
				if (spawnable) bakudans.push_back(b);
				// タイマーをゼロにリセット
				bakudanTimer = 0;
			}






			// 弾と敵・アイテム・爆弾の当たり判定
			for (auto& b : bullets) {
				if (!b.isAlive) continue;

				// 敵との衝突
				for (auto& e : komoris) {
					if (!e.isAlive) continue;
					float dx = b.pos.x - e.pos.x;
					float dy = b.pos.y - e.pos.y;
					float dist = sqrtf(dx * dx + dy * dy);
					if (dist < b.radius + e.radius) {
						b.isAlive = false;
						e.hp--;
						if (e.hp <= 0) e.isAlive = false;
					}
				}

				// 爆弾との衝突
				for (auto& baku : bakudans) {
					if (!baku.isAlive) continue;
					float dx = b.pos.x - baku.pos.x;
					float dy = b.pos.y - baku.pos.y;
					float dist = sqrtf(dx * dx + dy * dy);
					if (dist < b.radius + baku.radius) {
						b.isAlive = false;
						baku.hp--;
						if (baku.hp <= 0) {
							baku.isAlive = false;
							lives--; // ライフを減らす
							if (lives <= 0) {
								scene = OVER; // ゲームオーバーへ
							}
						}
					}
				}


				// 連射速度アップアイテムとの衝突
				for (auto& p : powerUps) {
					if (!p.isAlive) continue;
					float dx = b.pos.x - p.pos.x;
					float dy = b.pos.y - p.pos.y;
					float dist = sqrtf(dx * dx + dy * dy);
					if (dist < b.radius + p.radius) {
						b.isAlive = false;
						p.hp--;
						if (p.hp <= 0) {
							p.isAlive = false;
							// レベルに応じて連射速度を短縮
							powerUpLevel++;
							if (powerUpLevel == 1) defaultShotCooldown = 10;
							else if (powerUpLevel == 2) defaultShotCooldown = 8;
							else if (powerUpLevel == 3) defaultShotCooldown = 6;
							else defaultShotCooldown = 4;
						}
					}
				}

				// ショットガンアイテムとの衝突
				for (auto& s : shotgunPowerUps) {
					if (!s.isAlive) continue;
					float dx = b.pos.x - s.pos.x;
					float dy = b.pos.y - s.pos.y;
					float dist = sqrtf(dx * dx + dy * dy);
					if (dist < b.radius + s.radius) {
						b.isAlive = false;
						s.hp--;
						if (s.hp <= 0) {
							s.isAlive = false;
							player.shotgunLevel++;
							player.shotgunTimer = 6 * framePerSecond; // 効果6秒間
						}
					}
				}
			}

			// ショットガン効果時間を減らす
			if (player.shotgunTimer > 0) {
				player.shotgunTimer--;
				if (player.shotgunTimer <= 0) {
					player.shotgunLevel = 0; // 効果終了
				}
			}

			// 不要になったオブジェクトをvectorから削除（ガベージコレクション）
			bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](const Bullet& b) {
				return !b.isAlive;
				}), bullets.end());

			komoris.erase(std::remove_if(komoris.begin(), komoris.end(), [](const Enemy& e) {
				return !e.isAlive;
				}), komoris.end());

			bakudans.erase(std::remove_if(bakudans.begin(), bakudans.end(), [](const Bakudan& baku) {
				return !baku.isAlive;
				}), bakudans.end());

			powerUps.erase(std::remove_if(powerUps.begin(), powerUps.end(), [](const PowerUp& p) {
				return !p.isAlive;
				}), powerUps.end());

			shotgunPowerUps.erase(std::remove_if(shotgunPowerUps.begin(), shotgunPowerUps.end(), [](const ShotgunPowerUp& s) {
				return !s.isAlive;
				}), shotgunPowerUps.end());



		}break;


		case CLEAR: // ゲームクリア
			if (preKeys[DIK_RETURN] == 0 && keys[DIK_RETURN] != 0) {
				scene = TITLE;
				initGame();
			}
			break;

		case OVER: // ゲームオーバー
			if (preKeys[DIK_RETURN] == 0 && keys[DIK_RETURN] != 0) {
				scene = TITLE;
				initGame();
			}
			break;
		}

		/// =====================================================================================================
		/// 描画処理
		/// =====================================================================================================
		/*int elapsedMinutes = gameTimer / framePerSecond / 60;
		int elapsedSeconds = gameTimer / framePerSecond % 60;*/

		switch (scene) {
		case TITLE:
			Novice::DrawSprite(0, 0, tilteHandle, 1.0f, 1.0f, 0.0f, WHITE);
			break;
		case EXPLANATION1:
			Novice::DrawSprite(0, 0, Explanation1, 1.0f, 1.0f, 0.0f, WHITE);
			break;
		case EXPLANATION2:
			Novice::DrawSprite(0, 0, Explanation2, 1.0f, 1.0f, 0.0f, WHITE);
			break;
		case EXPLANATION3:
			Novice::DrawSprite(0, 0, Explanation3, 1.0f, 1.0f, 0.0f, WHITE);
			break;
		case SELECTION:
			Novice::DrawSprite(0, 0, selectHandle, 1.0f, 1.0f, 0.0f, WHITE);
			break;



		case GAME1://ステージ１----------------------------------------------------------------------------------------------------------------------



			//ステージ
			Novice::DrawSprite(0, 0, stageHandle, 1.0f, 1.0f, 0.0f, WHITE);

			//リソースタイマーの更新-------------------------------------------------------
			// 桁ごとに分解
			int numbersArray[2];
			numbersArray[0] = seconds / 10; // 十の位
			numbersArray[1] = seconds % 10; // 一の位

			for (int i = 0; i < 2; i++) {
				Novice::DrawSprite(
					graphWidth * i, 0,
					numberGrahs[numbersArray[i]],
					0.5f, 0.5f, 0.0f, WHITE
				);
			}
			//-----------------------------------------------------------------------------



			//魔法陣の耐久値
			Novice::DrawSprite(10, 60, tateHandle, 1.0f, 1.0f, 0.0f, WHITE);

			// プレイヤー描画
			Novice::DrawSprite((int)player.pos.x, (int)player.pos.y, playerHandle, 1.0f, 1.0f, 0.0f, WHITE);
			// 弾描画
			for (auto& b : bullets) {
				if (b.isAlive) {
					Novice::DrawSprite((int)b.pos.x, (int)b.pos.y, mahoudanHandle, 1.0f, 1.0f, 0.0f, WHITE);
				}
			}
			// ゴブリン描画
			for (auto& e : goburins) {
				if (e.isAlive) {
					Novice::DrawSprite((int)e.pos.x, (int)e.pos.y, goburinHandle, 1.0f, 1.0f, 0.0f, WHITE);
					Novice::ScreenPrintf((int)e.pos.x - 10, (int)e.pos.y - 30, "HP:%d", e.hp);
					Novice::DrawSprite((int)e.pos.x - 10, (int)e.pos.y - 30, hatoHandle, 0.4f, 0.4f, 0.0f, WHITE);
				}
			}
			// 連射速度アップアイテム描画
			for (auto& p : powerUps) {
				if (p.isAlive) {
					Novice::DrawSprite((int)p.pos.x, (int)p.pos.y, itemHandle, 0.7f, 0.6f, 0.0f, WHITE);
					Novice::ScreenPrintf((int)p.pos.x - 10, (int)p.pos.y - 30, "HP:%d", p.hp);
				}
			}
			//弾増加アップアイテム描画
			for (auto& s : shotgunPowerUps) {
				if (s.isAlive) {
					Novice::DrawSprite((int)s.pos.x, (int)s.pos.y, itemHandle, 0.7f, 0.6f, 0.0f, WHITE);
					Novice::ScreenPrintf((int)s.pos.x - 10, (int)s.pos.y - 30, "HP:%d", s.hp);
				}
			}
			Novice::ScreenPrintf(20, 20, "Lives: %d", lives);
			//Novice::ScreenPrintf(20, 40, "Time: %d:%02d", elapsedMinutes, elapsedSeconds);
			//Novice::ScreenPrintf(20, 60, "Shot Speed Level: %d", powerUpLevel);
			//Novice::ScreenPrintf(20, 80, "Shotgun Level: %d", player.shotgunLevel);
			//Novice::ScreenPrintf(20, 100, "Shotgun Timer: %d", player.shotgunTimer / framePerSecond);
			//Novice::ScreenPrintf(20, 120, "Seconds: %d", seconds);
			break;

		case GAME2://ステージ２---------------------------------------------------------------------------------------------------------------------------------------
			//ステージ
			Novice::DrawSprite(0, 0, stageHandle, 1.0f, 1.0f, 0.0f, WHITE);

			//リソースタイマーの更新-------------------------------------------------------
			// 桁ごとに分解
			numbersArray[0] = seconds / 10; // 十の位
			numbersArray[1] = seconds % 10; // 一の位

			for (int i = 0; i < 2; i++) {
				Novice::DrawSprite(
					graphWidth * i, 0,
					numberGrahs[numbersArray[i]],
					0.5f, 0.5f, 0.0f, WHITE
				);
			}
			//-----------------------------------------------------------------------------

			// プレイヤー描画
			Novice::DrawSprite((int)player.pos.x, (int)player.pos.y, playerHandle, 1.0f, 1.0f, 0.0f, WHITE);
			// 弾描画
			for (auto& b : bullets) {
				if (b.isAlive) {
					Novice::DrawSprite((int)b.pos.x, (int)b.pos.y, mahoudanHandle, 1.0f, 1.0f, 0.0f, WHITE);
				}
			}

			// コウモリ描画
			for (auto& e : komoris) {
				if (e.isAlive) {
					Novice::DrawSprite((int)e.pos.x, (int)e.pos.y, komoriHandle, 1.0f, 1.0f, 0.0f, WHITE);
					Novice::ScreenPrintf((int)e.pos.x - 10, (int)e.pos.y - 30, "HP:%d", e.hp);
					Novice::DrawSprite((int)e.pos.x - 10, (int)e.pos.y - 30, hatoHandle, 0.4f, 0.4f, 0.0f, WHITE);
				}
			}
			//爆弾描画
			for (auto& b : bakudans) {
				if (b.isAlive) {
					Novice::DrawSprite((int)b.pos.x, (int)b.pos.y, bakudanHandle, 1.0f, 1.0f, 0.0f, WHITE);
					Novice::ScreenPrintf((int)b.pos.x - 10, (int)b.pos.y - 30, "HP:%d", b.hp);
					Novice::DrawSprite((int)b.pos.x - 10, (int)b.pos.y - 30, hatoHandle, 0.4f, 0.4f, 0.0f, WHITE);
				}
			}


			// 連射速度アップアイテム描画
			for (auto& p : powerUps) {
				if (p.isAlive) {
					Novice::DrawSprite((int)p.pos.x, (int)p.pos.y, itemHandle, 0.7f, 0.6f, 0.0f, WHITE);
					Novice::ScreenPrintf((int)p.pos.x - 10, (int)p.pos.y - 30, "HP:%d", p.hp);
				}
			}
			//弾増加アップアイテム描画
			for (auto& s : shotgunPowerUps) {
				if (s.isAlive) {
					Novice::DrawSprite((int)s.pos.x, (int)s.pos.y, itemHandle, 0.7f, 0.6f, 0.0f, WHITE);
					Novice::ScreenPrintf((int)s.pos.x - 10, (int)s.pos.y - 30, "HP:%d", s.hp);
				}
			}


			Novice::ScreenPrintf(20, 20, "Lives: %d", lives);

			break;
		case GAME3://ステージ３

			break;

		case CLEAR:
			Novice::DrawSprite(0, 0, clearHandle, 1.0f, 1.0f, 0.0f, WHITE);
			break;

		case OVER:
			Novice::DrawSprite(0, 0, overHandle, 1.0f, 1.0f, 0.0f, WHITE);
			break;
		}

		Novice::EndFrame();
		if (preKeys[DIK_ESCAPE] == 0 && keys[DIK_ESCAPE] != 0) {
			break;
		}
	}
	Novice::Finalize();
	return 0;
}