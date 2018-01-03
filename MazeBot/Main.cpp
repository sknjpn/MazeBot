# include <Siv3D.hpp>

/*
仕様
tile	32x32
field	16x16
大きさ	512x512

window	1280x720
*/

struct Field;

enum struct Angle
{
	Right,
	Down,
	Left,
	Up
};

enum struct TType
{
	None,
	Wall,
	Start,
	Goal,
};

struct Tile
{
	TType	tType;
	Point	pos;
	Field*	field;

	Tile(Field* _field)
		: tType(TType::Wall)
		, pos(0, 0)
		, field(_field)
	{}

	Tile*	getTileTo(Angle _angle);
};

enum struct Word
{
	Wait,		//指定の時間待つ
	MoveForward,//前に進む
	TurnRight,	//右に曲がる
	TurnLeft,	//左に曲がる
	JumpTo,		//プログラムメモリジャンプ
	IsWall,		//前が壁なら指定の番地にジャンプ
};

struct Code
{
	Word		word;
	Array<int>	values;

	Code(Word _word)
		: word(_word)
	{}
};

struct Program
{
	bool	end;
	bool	error;	//Stop
	int		nowPos;		//実行中のコード
	double	t;			//実行中のプログレス
	Array<Code> codes;	//保持しているコード

	Code	nowCode() const
	{
		return codes.at(nowPos);
	}
};

struct Robot
{
	Field*	field;	//おかれているフィールド
	Angle	angle;
	Point	pos;
	Program	program;
	double	timer;

	Robot(Field* _field)
		: field(_field)
	{}

	void	draw();
	void	update(double _t)
	{
		if (program.error || program.end) return;
		program.t += _t;
		timer += _t;

		for (int i = 0;; i++)
		{
			if (program.nowPos >= int(program.codes.size())) program.nowPos = 0;

			if (i > 100)
			{
				program.error = true;
				break;
			}

			if (tile()->tType == TType::Goal)
			{
				program.end = true;
				break;
			}

			auto nc = program.nowCode();

			switch (nc.word)
			{
			case Word::Wait:
				if (nc.values.size() > 0)
				{
					if (program.t >= 1.0*nc.values[0])
					{
						program.t -= 1.0*nc.values[0];
						program.nowPos++;
					}
					else return;
				}
				else program.nowPos++;

				break;
			case Word::MoveForward:
				if (program.t >= 1.0)
				{
					program.t -= 1.0;
					if (tile()->getTileTo(angle)->tType != TType::Wall)
					{
						pos = tile()->getTileTo(angle)->pos;
					}
					program.nowPos++;
				}
				else return;

				break;
			case Word::TurnRight:
				if (program.t >= 1.0)
				{
					program.t -= 1.0;
					angle = Angle((int(angle) + 1) % 4);
					program.nowPos++;
				}
				else return;

				break;
			case Word::TurnLeft:
				if (program.t >= 1.0)
				{
					program.t -= 1.0;
					angle = Angle((int(angle) + 3) % 4);
					program.nowPos++;
				}
				else return;

				break;
			case Word::JumpTo:
				if (nc.values.size() > 0) program.nowPos = nc.values[0];
				break;
			case Word::IsWall:
				if (tile()->getTileTo(angle)->tType == TType::Wall)
				{
					if (nc.values.size() > 0) program.nowPos = nc.values[0];
				}
				else program.nowPos++;

				break;
			default:
				break;
			}

		}
	}
	Tile*	tile();
};

struct Field
{
	Robot	robot;
	Grid<Tile>	tiles;

	Field()
		: robot(this)
	{
		tiles.resize(16, 16, this);

		//座標を代入
		for (auto p : step(tiles.size())) tiles[p.y][p.x].pos = p;

		//ランダムな迷路を作成
		setRandomMaze();

		setCommand(L"");
	}

	void	setCommand(const String& _text)
	{
		setRandomMaze();

		auto& rp = robot.program;

		rp.error = false;
		rp.end = false;
		rp.codes.clear();
		robot.timer = 0.0;

		for (auto s : _text.remove(L"\n").split(L';'))
		{

			if (s.startsWith(L"MoveForward"))
			{
				rp.codes.emplace_back(Word::MoveForward);
			}
			else if (s.startsWith(L"TurnRight"))
			{
				rp.codes.emplace_back(Word::TurnRight);
			}
			else if (s.startsWith(L"TurnLeft"))
			{
				rp.codes.emplace_back(Word::TurnLeft);
			}
			else if (s.startsWith(L"Wait"))
			{
				rp.codes.emplace_back(Word::Wait);

				for (auto w : s.split(L','))
				{
					if (w == Format(Parse<int>(w))) rp.codes.back().values.emplace_back(Parse<int>(w));
				}
			}
			else if (s.startsWith(L"JumpTo"))
			{
				rp.codes.emplace_back(Word::JumpTo);

				for (auto w : s.split(L','))
				{
					if (w == Format(Parse<int>(w))) rp.codes.back().values.emplace_back(Parse<int>(w));
				}
			}
			else if (s.startsWith(L"IsWall"))
			{
				rp.codes.emplace_back(Word::IsWall);

				for (auto w : s.split(L','))
				{
					if (w == Format(Parse<int>(w))) rp.codes.back().values.emplace_back(Parse<int>(w));
				}
			}
			else Println(s);
		}

		if (rp.codes.empty()) rp.error = true;

		rp.nowPos = 0;

		robot.pos = getStartTile()->pos;
		robot.angle = Angle::Right;
	}
	void	setRandomMaze()
	{
		//壁を配置
		for (auto p : step(tiles.size()))
		{
			auto& t = tiles[p.y][p.x];

			t.tType = TType::Wall;
		}

		//床を配置
		for (auto p : step(Point(1, 1), tiles.size().movedBy(-3, -3)))
		{
			auto& t = tiles[p.y][p.x];

			t.tType = TType::None;
		}

		//棒を配置
		for (int x = 2; x < int(tiles.size().x - 2); x += 2)
		{
			for (int y = 2; y < int(tiles.size().y - 2); y += 2)
			{
				auto& t = tiles[y][x];

				t.tType = TType::Wall;
			}
		}

		//棒を倒す
		for (int x = 2; x < int(tiles.size().x - 2); x += 2)
		{
			for (int y = 2; y < int(tiles.size().y - 2); y += 2)
			{
				auto& t = tiles[y][x];

				for (;;)
				{
					auto angle = Angle(Random(y == 2 ? 3 : 2));
					if (t.getTileTo(angle)->tType == TType::None)
					{
						t.getTileTo(angle)->tType = TType::Wall;
						break;
					}
				}
			}
		}

		//Startを配置
		tiles.at(Point(1, 1)).tType = TType::Start;

		//Goalを配置
		tiles.at(Point(tiles.size().movedBy(-3, -3))).tType = TType::Goal;
	}
	void	update(double _t)
	{
		robot.update(_t);
	}
	void	draw()
	{
		for (auto p : step(tiles.size()))
		{
			auto& t = tiles[p.y][p.x];

			Rect rect(Point(t.pos * 32).movedBy(128, 128), 32, 32);

			switch (t.tType)
			{
			case TType::None:	rect.draw(Palette::Lightgrey);	break;
			case TType::Wall:	rect.draw(Palette::Gray);		break;
			case TType::Start:	rect.draw(Palette::Blue);		break;
			case TType::Goal:	rect.draw(Palette::Red);		break;
			}
		}

		//Robotの描画
		{
			Vec2 pos(robot.pos);
			double ang = int(robot.angle);

			if (!robot.program.error && !robot.program.end)
			{
				switch (robot.program.nowCode().word)
				{
				case Word::MoveForward:
					if (robot.tile()->getTileTo(robot.angle)->tType != TType::Wall) pos = robot.pos.lerp(robot.tile()->getTileTo(robot.angle)->pos, robot.program.t);
					break;
				case Word::TurnRight:
					ang += robot.program.t;
					break;
				case Word::TurnLeft:
					ang -= robot.program.t;
					break;
				}
			}

			Circle((pos * 32).movedBy(128, 128).movedBy(16, 16), 12).draw(Palette::Yellow).drawFrame(1, 1, Palette::Black);
			Circle((pos * 32).movedBy(Vec2(12, 0).rotated((ang - 0.4) * 90_deg).asPoint()).movedBy(128, 128).movedBy(16, 16), 4).draw(Palette::Black).drawFrame(1, 1, Palette::Black);
			Circle((pos * 32).movedBy(Vec2(12, 0).rotated((ang + 0.4) * 90_deg).asPoint()).movedBy(128, 128).movedBy(16, 16), 4).draw(Palette::Black).drawFrame(1, 1, Palette::Black);

			Circle((pos * 32).movedBy(128, 128).movedBy(16, 16), 4).draw(Palette::Blue).drawFrame(1, 1, Palette::Black);
			if (robot.program.error) Circle((pos * 32).movedBy(128, 128).movedBy(16, 16), 4).draw(Palette::Red).drawFrame(1, 1, Palette::Black);
			if (robot.program.end) Circle((pos * 32).movedBy(128, 128).movedBy(16, 16), 4).draw(Palette::Green).drawFrame(1, 1, Palette::Black);
		}
	}
	Tile*	getStartTile()
	{
		for (auto p : step(tiles.size()))
		{
			if (tiles[p.y][p.x].tType == TType::Start) return &tiles[p.y][p.x];
		}
		return nullptr;
	}
	Tile*	getGoalTile()
	{
		for (auto p : step(tiles.size()))
		{
			if (tiles[p.y][p.x].tType == TType::Goal) return &tiles[p.y][p.x];
		}
		return nullptr;
	}
};

Tile*	Robot::tile()
{
	return &field->tiles[pos.y][pos.x];
}

Tile*	Tile::getTileTo(Angle _angle)
{
	auto p = pos;
	switch (_angle)
	{
	case Angle::Right:	p.moveBy(1, 0);	 break;
	case Angle::Down:	p.moveBy(0, 1);	 break;
	case Angle::Left:	p.moveBy(-1, 0); break;
	case Angle::Up:		p.moveBy(0, -1); break;
	default:	return nullptr;	break;
	}

	//tilesのサイズ
	auto size = field->tiles.size();

	//参照できる範囲内かどうか
	if (p.x <= -1 || p.y <= -1 || p.x >= size.x || p.y >= size.y) return nullptr;

	return &field->tiles.at(p);
}

void Main()
{
	Graphics::SetBackground(ColorF(0.8, 0.9, 1.0));

	Window::SetTitle(L"MazeBot");

	Window::Resize(1280, 720);

	Field field;

	GUI gui(GUIStyle::Default);

	const Font font(30);

	gui.setTitle(L"マイコン");

	// 最大 6 文字に設定
	gui.add(L"text", GUITextArea::Create(12, 12, 140));
	gui.add(GUIHorizontalLine::Create());
	gui.add(L"ms", GUIText::Create(L"", 100));
	gui.add(GUIHorizontalLine::Create());
	gui.add(L"run", GUIButton::Create(L"実行"));
	gui.add(L"reset", GUIButton::Create(L"リセット"));

	gui.setCenter(Point(Window::Size().x * 3 / 4, Window::Size().y / 2));
	gui.textArea(L"text").setText(L"IsWall,2;\nJumpTo,4;\nTurnRight;\nJumpTo,0;\nMoveForward;\nTurnLeft;");


	while (System::Update())
	{
		field.update(Input::KeyShift.pressed ? 1.0 : 0.1);

		field.draw();
		gui.text(L"ms").text = Format(L"経過時間: ", field.robot.timer,L"ms");

		if (gui.button(L"run").pushed)
		{
			field.setCommand(gui.textArea(L"text").text);
		}
		if (gui.button(L"reset").pushed)
		{
			gui.textArea(L"text").setText(L"");
		}
	}
}
