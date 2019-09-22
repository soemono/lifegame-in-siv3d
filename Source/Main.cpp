
# include <Siv3D.hpp> // OpenSiv3D v0.4.0

//セルの生死を決めるルール用構造体
struct automatonrule {
	Array<bool> live = { 0,0,1,1,0,0,0,0,0 };
	Array<bool> dead = { 0,0,0,1,0,0,0,0,0 };
};

//初期化用関数,変数以外はなるべくここに入れよう
void init(const int16 &width,const int16 &height,const int16 &ntimes) {
	//ウィンドウの名前を変更
	Window::SetTitle(U"lifegame");

	//ウィンドウの初期サイズを適用
	Window::Resize(Size(width * ntimes, height * ntimes));

	//背景色を変更
	Scene::SetBackground(ColorF(0.05, 0.05, 0.05));
}

//fieldの状態を見て作ったフィールドをnextfieldに渡します
void fieldupdate(Grid<bool>& field, Grid<bool>& nextfield, automatonrule Rule, const int16& height, const int16& width, const int16& outofFrame) {
	for (auto y : step(height + ((outofFrame - 1) * 2))) {
		for (auto x : step(width + ((outofFrame - 1) * 2))) {
			int8 count = 0;

			//番兵を置いてあるためちょっとオフセットしてある
			int32 ysum = y + 1;
			int32 xsum = x + 1;

			//周囲のセルをカウント
			if (field[ysum - 1][xsum - 1]) ++count;
			if (field[ysum - 1][xsum]) ++count;
			if (field[ysum - 1][xsum + 1]) ++count;
			if (field[ysum][xsum - 1]) ++count;
			if (field[ysum][xsum + 1]) ++count;
			if (field[ysum + 1][xsum - 1]) ++count;
			if (field[ysum + 1][xsum]) ++count;
			if (field[ysum + 1][xsum + 1]) ++count;

			//配列でセルの生死をいじれるようになっている
			nextfield[ysum][xsum] = (
				field[ysum][xsum] && (
					(count == 0 && Rule.live[0]) ||
					(count == 1 && Rule.live[1]) ||
					(count == 2 && Rule.live[2]) ||
					(count == 3 && Rule.live[3]) ||
					(count == 4 && Rule.live[4]) ||
					(count == 5 && Rule.live[5]) ||
					(count == 6 && Rule.live[6]) ||
					(count == 7 && Rule.live[7]) ||
					(count == 8 && Rule.live[8])
				) || !field[ysum][xsum] && (
					(count == 0 && Rule.dead[0]) ||
					(count == 1 && Rule.dead[1]) ||
					(count == 2 && Rule.dead[2]) ||
					(count == 3 && Rule.dead[3]) ||
					(count == 4 && Rule.dead[4]) ||
					(count == 5 && Rule.dead[5]) ||
					(count == 6 && Rule.dead[6]) ||
					(count == 7 && Rule.dead[7]) ||
					(count == 8 && Rule.dead[8])
				)
				);
		}
	}
}

void Main()
{
	//グリッドの高さ、幅を変更
	constexpr int16 height = 250;
	constexpr int16 width = 400;

	//画面枠外領域の広さを設定
	constexpr int16 outofFrame = 3;

	//表示倍率
	int16 ntimes = 2;

	//フィールド、次回フィールド、色用フィールドなどを設定
	Grid<bool> field(width + (outofFrame * 2), height + (outofFrame * 2), false);
	Grid<bool> nextfield(width + (outofFrame * 2), height + (outofFrame * 2), false);
	Grid<bool> beforefield(width + (outofFrame * 2), height + (outofFrame * 2), false);
	Grid<double> colorfield(width, height, 0.0);

	//ライフゲーム描画用画像、テクスチャ
	Image image(width, height, ColorF(0,0,0,0));
	DynamicTexture texture(image);

	//グリッド描画用画像、テクスチャ
	Image gridimage(width * 4, height * 4, ColorF(0, 0, 0, 0));
	DynamicTexture gridtexture(gridimage);

	//スイッチ用変数
	bool toedit = false;
	bool editblush = false;
	bool fieldrandfill = false;
	bool fieldranddel = false;
	bool showoption = false;
	size_t xtimebutton = 1;
	bool randomthing = true;
	bool autostep = true;
	bool autosteplog = autostep;
	bool steptostep = false;
	bool windowresize = false;
	bool drawgrid = false;
	bool continuegrid = false;
	//速度系変数
	double colorspeed = 0.04;
	double colorspeedadjust = 0.0;
	double autostepspeed = 0.32;

	//スイッチ用アイコン
	const Texture iconPlay(Icon(0xf04b, 25));
	const Texture iconPause(Icon(0xf04c, 28));

	//描画更新用スイッチ
	bool updated = false;
	bool needdraw = true;

	//フォント
	Font fontsmall(14);
	Font fontnormal(20);

	//汎用ストップウォッチ
	Stopwatch stopwatch(true);

	//ルール定義構造体
	automatonrule Rule;

	//初期化用関数
	init(width, height, ntimes);

	//線形補間を無効にする、メインループ内にあると例外吐いて落ちたのでここに配置
	//initに入れるとうまく反映されないのでここに配置しときます
	ScopedRenderStates2D sampler(SamplerState::ClampNearest);

	while (System::Update())
	{

		//ライフゲーム動作部分
		if ((autostep || steptostep) && stopwatch.sF() >= (autostepspeed * autostepspeed * autostepspeed)) {
			stopwatch.restart();

			//フィールドランダム埋め、連打するとセルが増えていくようにしてある
			if (fieldrandfill) {
				for (auto y : step(height)) {
					for (auto x : step(width)) {
						if (RandomBool(0.2)) {
							field[y + outofFrame][x + outofFrame] = true;
						}
					}
				}
				fieldrandfill = false;
			}

			if (fieldranddel) {
				for (auto y : step(height)) {
					for (auto x : step(width)) {
						if (RandomBool(0.2)) {
							field[y + outofFrame][x + outofFrame] = false;
						}
					}
				}
				fieldranddel = false;
			}

			//ランダムなぼわぼわが下に出る
			if (randomthing) {
				for (auto y : step(3)) {
					for (auto x : step(width)) {
						field[height + outofFrame - y][x + 1] = RandomBool(0.5);
					}
				}
			}

			//ライフゲーム部
			fieldupdate(field,nextfield,Rule,height,width,outofFrame);

			//色フィールド設定部	
			for (auto y : step(height)) {
				for (auto x : step(width)) {
					if (field[y + outofFrame][x + outofFrame] == true) {
						colorfield[y][x] = 1.0;
					}
					else {
						if (colorfield[y][x] > 0) {
							//colorfieldはスライダーによって変更できる変数colorspeedとcoloradjustによって減少する
							double speed = colorspeed + colorspeedadjust;
							colorfield[y][x] -= (speed * speed);
						} else if (colorfield[y][x] != 0) {
							colorfield[y][x] = 0;
						}
					}
				}
			}

			updated = true;
		}

		//フィールド更新
		if (autostep || steptostep) {
			beforefield = field;
			field = nextfield;
			steptostep = false;
		}

		//エディット処理

		int32 cursorx = (Cursor::Pos().x / ntimes);
		int32 cursory = (Cursor::Pos().y / ntimes);

		if (toedit) {
			ClearPrint();
			Print(U"Enterで抜ける");
			//枠外に出たときの変数
			
			bool outofrange = (cursorx < 0 || cursorx >= width || cursory < 0 || cursory >= height);

			if (!outofrange) {
				if (MouseL.down()) {
					editblush = !field[cursory + outofFrame][cursorx + outofFrame];
				}

				if (MouseL.pressed()) {
					if (editblush) {
						image[cursory][cursorx] = ColorF(Palette::White);
					}
					else {
						if (colorfield[cursory][cursorx] > 0) {
							image[cursory][cursorx] = HSV(213, 1, 1, colorfield[cursory][cursorx]);
						}
						else {
							image[cursory][cursorx] = ColorF(0, 0, 0, 0);
						}

						if (beforefield[cursory + outofFrame][cursorx + outofFrame]) {
							image[cursory][cursorx] = ColorF(Palette::Pink);
						}
					}
					field[cursory + outofFrame][cursorx + outofFrame] = editblush;
					needdraw = true;
				}
			}
		}

		//描画部
		if (updated) {

			for (auto y : step(height)) {
				for (auto x : step(width)) {
					int32 ysum = y + outofFrame;
					int32 xsum = x + outofFrame;
					if (beforefield[ysum][xsum] || field[ysum][xsum]) {
						image[y][x] = field[ysum][xsum] ? Color(Palette::White) : Color(Palette::Pink);
					}
					else if (colorfield[y][x] > 0) {
						image[y][x] = HSV(213,1, 1,colorfield[y][x]);
					}
					else {
						image[y][x] = ColorF(0, 0, 0, 0);
					}
				}
			}
			updated = false;
			needdraw = true;
		}

		//グリッド描画
		if (drawgrid || windowresize) {
			//グリッドを作成する
			constexpr ColorF gridcolor(0.5, 0.5, 0.5, 0.3);

			for (int32 y : step(height* ntimes)) {
				if ((y % ntimes) == 0) {
					for (int32 x : step(width* ntimes)) {
						gridimage[y][x] = gridcolor;
					}
				}
				else {
					for (int32 x : step(width * ntimes)) {
						if (x % ntimes == 0) {
							gridimage[y][x] = gridcolor;
						}
						else {
							gridimage[y][x] = ColorF(0, 0, 0, 0);
						}
					}
				}
			}
			drawgrid = false;
			gridtexture.fill(gridimage);
		}

		if (needdraw) {
			//テクスチャをimageに書き換え
			texture.fill(image);

			//n倍で描画
			texture.scaled(ntimes).draw();

			//グリッドを描画する
			if (continuegrid && ntimes >= 2) {
				gridtexture.draw();
			}
		}

		
		

		//描画したあとにウィンドウサイズの更新を行う
		if (windowresize) {
			Window::Resize(Size(width * ntimes, height * ntimes));
			windowresize = false;
		}
		//----------キー設定-------------
		
		//スペースキーで動作反転
		if (KeySpace.down() && !toedit) {
			autostep = !autostep;
		}

		//Zキーで一コマ動作
		if (KeyZ.down() && !toedit && !autostep) {
			steptostep = true;
		}

		//Enterキーでエディット入ったり出たり
		if (KeyEnter.down()) {
			ClearPrint();
			toedit = !toedit;
			if (toedit) {
				autosteplog = autostep;
				autostep = false;
			}
			if (!toedit) {
				autostep = autosteplog;
			}
		}


		//---------------------GUI設定------------------ 
		//ウィンドウにマウスを当てたときだけ出る
		Rect window(0, 0, width* ntimes,height* ntimes);
		if (window.mouseOver()) {
			//表示1倍の時の小さいGUI
			if (ntimes == 1 && !toedit) {
				RoundRect(5, 5, 160, 85, 5).draw();

				//スタートストップボタン
				constexpr int32 Playbuttonposx = 10;
				constexpr int32 Playbuttonposy = 11;
				RoundRect Playbutton(Playbuttonposx, Playbuttonposy, 35, 34, 5);
				Playbutton.draw();
				if (Playbutton.mouseOver()) {
					Playbutton.draw(ColorF(0.1, 0.1, 0.1, 0.1));
				}
				Playbutton.drawFrame(0, 1, ColorF(0.7));
				if (autostep) {
					iconPause.draw(Point(Playbuttonposx + 5, Playbuttonposy + 4), ColorF(0.25));
				}
				else {
					iconPlay.draw(Point(Playbuttonposx + 7, Playbuttonposy + 4), ColorF(0.25));
				}
				if (Playbutton.mouseOver() && MouseL.down()) {
					autostep = !autostep;
				}

				//2倍復帰ボタン
				if (SimpleGUI::Button(U"2倍に戻す", Vec2(50, 10),110)) {
					ntimes = 2;
					xtimebutton = 1;
					windowresize = true;
				}

				SimpleGUI::Slider(U"",autostepspeed, 1, 0.01, Vec2(10, 50),0,150);
				

			} else if (!showoption && !toedit) {
				//==================デフォルトGUI=======================

				//------------------左端揃えボタン---------------------
				
				//枠の角丸長方形
				RoundRect(5, 5, 265, 206, 5).draw();

				//スタートストップボタン
				constexpr int32 Playbuttonposx = 11;
				constexpr int32 Playbuttonposy = 11;
				RoundRect Playbutton(Playbuttonposx, Playbuttonposy , 35, 34, 5);
				Playbutton.draw();
				if (Playbutton.mouseOver()) {
					Playbutton.draw(ColorF(0.1,0.1,0.1,0.1));
				}
				Playbutton.drawFrame(0,1,ColorF(0.7));
				if (autostep) {
					iconPause.draw(Point(Playbuttonposx + 5, Playbuttonposy + 4), ColorF(0.25));
				} else {
					iconPlay.draw(Point(Playbuttonposx + 7, Playbuttonposy + 4), ColorF(0.25));
				}
				if (Playbutton.mouseOver() && MouseL.down()) {
					autostep = !autostep;
				}

				//1ステップづつ進むボタン（ライフゲームが止まっているときだけ出現する）
				if (SimpleGUI::Button(U"1コマ動かす", Vec2(51, 10), 120,!autostep) ) {
					steptostep = true;
				}

				//リセットボタン
				if (SimpleGUI::Button(U"全消去", Vec2(175, 10), 70)) {

					//フィールドの初期化
					for (auto y : step(height + (outofFrame * 2))) {
						for (auto x : step(width + outofFrame * 2)) {
							beforefield[y][x] = 0;
							field[y][x] = 0;
							nextfield[y][x] = 0;
						}
					}

					//画像と色の初期化
					for (auto y : step(height)) {
						for (auto x : step(width)) {
							image[y][x] = Color(Palette::Black);
							colorfield[y][x] = 0;
						}
					}
				}

				//フィールドをランダム初期値で埋める
				if (SimpleGUI::Button(U"ランダム埋め", Vec2(10, 50),125)) {
					fieldrandfill = true;
				}

				if (SimpleGUI::Button(U"ランダム消去", Vec2(140, 50), 125)) {
					fieldranddel = true;
				}

				//世代が変わるスピードの変更
				SimpleGUI::Slider(U"速度", autostepspeed, 1,0.01, Vec2(10, 90),50,190);

				//ランダムなやつ入り切りボタン
				SimpleGUI::CheckBox(randomthing, U"ランダム入力", Vec2(8, 125));

				//オプション入るボタン
				if (SimpleGUI::Button(U"オプション", Vec2(10, 170),125)) {
					showoption = true;
				}

				//エディット入るボタン
				if (SimpleGUI::Button(U"エディット", Vec2(140, 170),125)) {
					//前のログを残しておき、エディット抜けたあと前の状態に復帰する
					autosteplog = autostep;
					autostep = false;
					toedit = true;
				}

				//------------------右端揃えボタン---------------------
				//ルールを変えるチェックボックス
				RoundRect((width * ntimes) - 105, 5, 100, 312, 5).draw();
				fontsmall(U"生存ルール").draw((width * ntimes) - 80, 5, Color(0, 0, 0));
				fontsmall(U"live   dead").draw((width * ntimes) - 78, 21, Color(0, 0, 0));
				for (auto i : step(9)) {
					fontnormal(U"{}"_fmt(i)).draw((width * ntimes) - 97, 39 + (30 * i), Color(0, 0, 0));
					SimpleGUI::CheckBox(Rule.live[i], U"", Vec2((width * ntimes) - 85, 37 + (30 * i)), 40);
					SimpleGUI::CheckBox(Rule.dead[i], U"", Vec2((width * ntimes) - 45, 37 + (30 * i)), 40);
				}
			}
			else if (showoption && !toedit) {
				//===============オプション設定GUI==============
				RoundRect(5, 5, 250, 295, 5).draw();

				if (SimpleGUI::CheckBox(continuegrid, U"グリッド表示", Vec2(10, 10))) {
					drawgrid = true;
				}

				//青色の消える速度変えスライダー
				if (SimpleGUI::Slider(U"大まか", colorspeed, 0.0, 0.5, Vec2(10, 80), 70, 170)) {
					if (colorspeed == 0.5) {
						colorspeed = 1;
					}
				}

				SimpleGUI::Slider(U"微調整",colorspeedadjust,0,0.05,Vec2(10,110),70,170);
				fontnormal(U"・青色の減衰速度").draw(10, 50, Color(0, 0, 0));

				//表示倍率の変更を行うやつ
				if (SimpleGUI::RadioButtons(xtimebutton, { U"1倍",U"2倍",U"4倍" }, Vec2(10, 180), 80)) {
					uint16 numarray[4] = { 1,2,4 };
					ntimes = numarray[xtimebutton];
					windowresize = true;
				}
				fontnormal(U"・表示倍率").draw(10, 150, Color(0, 0, 0));

				//オプションから出るボタン
				if (SimpleGUI::Button(U"オプションから出る", Vec2(10, 305))) {
					showoption = false;
				}
			}
			//エディットのときはGUI無し
		}
	}
}
