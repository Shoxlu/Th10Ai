#include "Th10Ai/Path.h"


namespace th
{
	//Useful dirs when just the bot just did the first dir
	//idk how much doing "down" then "up" is bad, this feels like pre-computed greedy algorithm
	const DIR Path::FIND_DIRS[to_underlying(DIR::MAX_COUNT)][5] =
	{
		// DIR::HOLD
		{ DIR::HOLD,       DIR::UP,        DIR::DOWN,       DIR::LEFT,       DIR::RIGHT,      },//DIR::UP_LEFT, DIR::UP_RIGHT, DIR::DOWN_LEFT, DIR::DOWN_RIGHT },
		// DIR::UP
		{ DIR::UP,         DIR::UP_LEFT,   DIR::UP_RIGHT,   DIR::LEFT,       DIR::RIGHT,      },//DIR::HOLD,    DIR::HOLD,     DIR::HOLD,      DIR::HOLD      },
		// DIR::DOWN
		{ DIR::DOWN,       DIR::DOWN_LEFT, DIR::DOWN_RIGHT, DIR::LEFT,       DIR::RIGHT,      },//DIR::HOLD,    DIR::HOLD,     DIR::HOLD,      DIR::HOLD      },
		// DIR::LEFT
		{ DIR::LEFT,       DIR::UP_LEFT,   DIR::DOWN_LEFT,  DIR::UP,         DIR::DOWN,       },//DIR::HOLD,    DIR::HOLD,     DIR::HOLD,      DIR::HOLD      },
		// DIR::RIGHT
		{ DIR::RIGHT,      DIR::UP_RIGHT,  DIR::DOWN_RIGHT, DIR::UP,         DIR::DOWN,       },//DIR::HOLD,    DIR::HOLD,     DIR::HOLD,      DIR::HOLD      },
		// DIR::UP_LEFT
		{ DIR::UP_LEFT,    DIR::LEFT,      DIR::UP,         DIR::DOWN_LEFT,  DIR::UP_RIGHT,   },//DIR::HOLD,    DIR::HOLD,     DIR::HOLD,      DIR::HOLD      },
		// DIR::UP_RIGHT
		{ DIR::UP_RIGHT,   DIR::UP,        DIR::RIGHT,      DIR::UP_LEFT,    DIR::DOWN_RIGHT, },//DIR::HOLD,    DIR::HOLD,     DIR::HOLD,      DIR::HOLD      },
		// DIR::DOWN_LEFT
		{ DIR::DOWN_LEFT,  DIR::DOWN,      DIR::LEFT,       DIR::DOWN_RIGHT, DIR::UP_LEFT,    },//DIR::HOLD,    DIR::HOLD,     DIR::HOLD,      DIR::HOLD      },
		// DIR::DOWN_RIGHT
		{ DIR::DOWN_RIGHT, DIR::RIGHT,     DIR::DOWN,       DIR::UP_RIGHT,   DIR::DOWN_LEFT,  },//DIR::HOLD,    DIR::HOLD,     DIR::HOLD,      DIR::HOLD      }
	};

	const int_t Path::FIND_DIR_COUNTS[to_underlying(DIR::MAX_COUNT)] = { 1, 5, 5, 5, 5, 5, 5, 5, 5 };

	//const int_t Path::FIND_LIMIT = 120;
	//const float_t Path::FIND_DEPTH = 40;
	const vec2 Path::RESET_POS = vec2(_F(0.0), _F(375.0));

	Path::Path(Status& status, Scene* scenes,
		const std::optional<Item>& itemTarget,
		const std::optional<Bullet>& bulletTarget,
		const std::optional<Enemy>& enemyTarget,
		bool underEnemy, bool anyItems) :
		m_status(status),
		m_scenes(scenes),
		m_itemTarget(itemTarget),
		m_bulletTarget(bulletTarget),
		m_enemyTarget(enemyTarget),
		m_underEnemy(underEnemy),
		m_dir(DIR::HOLD),
		m_slowFirst(false),
		m_bestScore(std::numeric_limits<float_t>::lowest()),
		m_count(0),
		m_anyItems(anyItems)
	{
	}

	std::optional<Item> Path::findItem()
	{
		const Player& player = m_status.getPlayer();
		const std::vector<Item>& items = m_status.getItems();
		const std::vector<Enemy>& enemies = m_status.getEnemies();
		std::optional<Item> target;

		if (items.empty())
			return target;

		// 自机高于1/4屏
		if (player.pos.y < Scene::SIZE.y / 4)
		{
			// 进入冷却
			return target;
		}

		// 自机高于1/2屏，敌机多于5个
		if ((player.pos.y < Scene::SIZE.y / 2) && (enemies.size() > 5))
		{
			// 进入冷却
			return target;
		}

		float_t minDist = std::numeric_limits<float_t>::max();
		//float_t maxY = std::numeric_limits<float_t>::lowest();
		for (const Item& item : items)
		{
			// 道具在屏幕外
			if (!Scene::IsInScene(item.pos))
				continue;

			// 道具高于1/5屏
			if (item.pos.y < Scene::SIZE.y / 5)
				continue;

			// 道具不在自机1/4屏内
			float_t dy = std::abs(item.pos.y - player.pos.y);
			if (dy > Scene::SIZE.y / 4)
				continue;

			// 道具太靠近敌机
			bool tooClose = false;
			for (const Enemy& enemy : enemies)
			{
				if (item.distance(enemy) < 150)
				{
					tooClose = true;
					break;
				}
			}
			if (tooClose)
				continue;

			// 道具与自机距离最近
			float_t dist = item.distance(player);
			if (dist < minDist)
			{
				minDist = dist;
				target = item;
			}

			//if (item.y > maxY)
			//{
			//	maxY = item.y;
			//	target = item;
			//}
		}

		return target;
	}

	// 查找敌机
	std::optional<Enemy> Path::findEnemy()
	{
		const Player& player = m_status.getPlayer();
		const std::vector<Enemy>& enemies = m_status.getEnemies();
		std::optional<Enemy> target;

		if (enemies.empty())
			return target;

		// 自机高于1/4屏
		//if (player.pos.y < Scene::SIZE.y / 4)
		//	return target;

		float_t minDist = std::numeric_limits<float_t>::max();
		for (const Enemy& enemy : enemies)
		{
			// 敌机在屏幕外
			if (!Scene::IsInScene(enemy.pos))
				continue;

			// 敌机在自机下面
			//if (enemy.pos.y > player.pos.y)
			//	continue;

			// 敌机与自机X轴距离最近
			float_t dx = std::abs(enemy.pos.x - player.pos.x);
			if (dx < minDist)
			{
				minDist = dx;
				target = enemy;
			}
		}

		return target;
	}
	std::optional<Bullet> Path::findBullet()
	{
		const Player& player = m_status.getPlayer();
		const std::vector<Bullet>& bullets = m_status.getBullets();
		std::optional<Bullet> target;

		if (bullets.empty())
			return target;

		float_t minDist = std::numeric_limits<float_t>::max();
		for (const Bullet& bullet : bullets)
		{
			if (!Scene::IsInScene(bullet.pos))
				continue;

			float_t dx = std::abs(bullet.pos.x - player.pos.x);
			if (dx < minDist)
			{
				minDist = dx;
				target = bullet;
			}
		}

		return target;
	}

	Result Path::find(DIR dir)
	{
		m_dir = dir;
		m_slowFirst = (!m_itemTarget.has_value() && m_underEnemy);

		Action action = {};
		action.fromPos = m_status.getPlayer().pos;
		action.fromDir = m_dir;
		action.frame = 1;
		action.score = 0;

		return dfs(action);
	}
	Result Path::findminmax(DIR dir)
	{
		m_dir = dir;
		m_slowFirst = (!m_itemTarget.has_value() && m_underEnemy);

		Action action = {};
		action.fromPos = m_status.getPlayer().pos;
		action.fromDir = m_dir;
		action.frame = 1;
		action.score = 0;
		Result res = minmax(action);
		m_bestScore = res.score;
		m_slowFirst = res.slow;
		return res;
	}
	bool Path::PlayerDies(const Action& action, Result& result, Player& player)
	{
		result.slow = m_slowFirst;
		player.setPosition(action.fromPos);
		player.move(action.fromDir, result.slow);
		RegionCollideResult rcr = {};
		if (!Scene::IsInPlayerRegion(player.pos)
			|| (rcr = m_scenes[action.frame - 1].collideAll(player)).collided)
		{
			result.slow = !m_slowFirst;
			player.setPosition(action.fromPos);
			player.move(action.fromDir, result.slow);
			
			if (!Scene::IsInPlayerRegion(player.pos)
				|| (rcr = m_scenes[action.frame - 1].collideAll(player)).collided)
			{
				return true;
			}
		}
		return false;
	}

	void Path::Update_status(Player& player, int_t i)
	{

	}

	float_t Path::HorizonScore(Result& result, Player& player)
	{
		//Pretty rare to die without any bullet but well..

		std::optional<Bullet> bulletTarget = m_bulletTarget;
		if (bulletTarget.has_value())
		{
			Vector2 nearest_bullet = bulletTarget.value().pos;
			result.score += CalcFarScore(player.pos, nearest_bullet) * _F(100.0);
		}
		return result.score;
	}
	
	float_t Path::Score(Result& result, Player& player)
	{
		std::optional<Item> itemTarget = m_itemTarget;
		std::optional<Enemy> enemyTarget = m_enemyTarget;
		result.valid = true;

		if (m_anyItems && !enemyTarget.has_value() && (!itemTarget.has_value() || (player.pos - itemTarget.value().pos).length() > player.pos.y - 120))
		{
			result.score += CalcNearScore(player.pos, Vector2(player.pos.x, 0)) * _F(100.0);
		}
		else if (itemTarget.has_value())
		{
			result.score += CalcNearScore(player.pos, itemTarget.value().pos) * _F(100.0);
		}
		else if (enemyTarget.has_value())
		{
			//result.score += CalcShootScore(player.pos, m_enemyTarget.value().pos) * _F(100.0);
			//result.score += CalcNearScore(player.pos, Vector2(m_enemyTarget.value().pos.x, RESET_POS.y)) * _F(100.0);
			result.score += CalcRelaxedNearScore(player.pos, Vector2(enemyTarget.value().pos.x, RESET_POS.y), _F(0.0)) * _F(100.0);
		}
		else
		{
			//result.score += CalcNearScore(player.pos, RESET_POS) * _F(100.0);
			result.score += CalcRelaxedNearScore(player.pos, RESET_POS, _F(64.0)) * _F(100.0);
		}
		return result.score;
	}

	//actually it's only a max, since min would always return the same thing...
	Result Path::minmax(const Action& action)
	{


		/*
		Si l'action tue le joueur: renvoyer un résultat invalide
		Si la depth/node_limit est dépassé, renvoyer le résultat munide son score
		Sinon: descendre en profondeur en testant chaque move et prendre le max de ceux calculés
		
		*/
		Result result = {.valid = true};
		++m_count;
		Player player = m_status.getPlayer();
		if (PlayerDies(action, result, player))
		{
			//Update_status(player, action.frame - 1);
			result.score += HorizonScore(result, player)-100000.0;
			return result;
		}
		else if (m_count > FIND_LIMIT || action.frame > FIND_DEPTH)
		{
			Score(result, player);
			result.valid = true;
			return result;
		}
		else
		{
			Result best_res = { .score = std::numeric_limits<int_t>::lowest()};
			for (int_t i = 0; i < FIND_DIR_COUNTS[to_underlying(m_dir)]; ++i)
			{
				DIR dir = FIND_DIRS[to_underlying(m_dir)][i];

				Action nextAct = {};
				nextAct.fromPos = player.pos;
				nextAct.fromDir = dir;
				nextAct.frame = action.frame + 1;

				Result nextRes = minmax(nextAct);

				if (best_res.score < nextRes.score)
				{
					best_res = nextRes;
				}
			}
			best_res.slow = result.slow;
			return best_res;
		}

	}


	Result Path::dfs(const Action& action)
	{
		Result result = {};

		// 超过搜索节点限制(Search Node Limit Exceeded)
		++m_count;
		if (m_count > FIND_LIMIT)
			return result;

		if (action.frame > FIND_DEPTH)
		{
			result.valid = true;
			return result;
		}

		// 前进到下一个坐标(Advance to the next coordinate.)
		Player player = m_status.getPlayer();
		player.setPosition(action.fromPos);
		player.move(action.fromDir, m_slowFirst);
		result.slow = m_slowFirst;
		RegionCollideResult rcr = {};
		if (!Scene::IsInPlayerRegion(player.pos)
			|| (rcr = m_scenes[action.frame - 1].collideAll(player)).collided)
		{
			player.setPosition(action.fromPos);
			player.move(action.fromDir, !m_slowFirst);
			result.slow = !m_slowFirst;
			if (!Scene::IsInPlayerRegion(player.pos)
				|| (rcr = m_scenes[action.frame - 1].collideAll(player)).collided)
			{
				return result;
			}
		}

		result.valid = true;

		if (m_anyItems && !m_enemyTarget.has_value() && (!m_itemTarget.has_value() || (player.pos - m_itemTarget.value().pos).length() > player.pos.y - 120))
		{
			result.score += CalcNearScore(player.pos, Vector2(player.pos.x, 115)) * _F(100.0);
		}
		else if (m_itemTarget.has_value())
		{
			result.score += CalcNearScore(player.pos, m_itemTarget.value().pos) * _F(100.0);
		}
		else if (m_enemyTarget.has_value())
		{
			//result.score += CalcShootScore(player.pos, m_enemyTarget.value().pos) * _F(100.0);
			//result.score += CalcNearScore(player.pos, Vector2(m_enemyTarget.value().pos.x, RESET_POS.y)) * _F(100.0);
			result.score += CalcRelaxedNearScore(player.pos, Vector2(m_enemyTarget.value().pos.x, RESET_POS.y), _F(16.0)) * _F(100.0);
		}
		else
		{
			//result.score += CalcNearScore(player.pos, RESET_POS) * _F(100.0);
			result.score += CalcRelaxedNearScore(player.pos, RESET_POS, _F(64.0)) * _F(100.0);
		}

		float_t total = action.score + result.score;
		float_t avg = total / action.frame;
		if (avg > m_bestScore)
		{
			m_bestScore = avg;
		}

		int_t nextValidCount = FIND_DIR_COUNTS[to_underlying(m_dir)];
		for (int_t i = 0; i < FIND_DIR_COUNTS[to_underlying(m_dir)]; ++i)
		{
			DIR dir = FIND_DIRS[to_underlying(m_dir)][i];

			Action nextAct = {};
			nextAct.fromPos = player.pos;
			nextAct.fromDir = dir;
			nextAct.frame = action.frame + 1;
			nextAct.score = total;

			Result nextRes = dfs(nextAct);

			if (m_count > FIND_LIMIT)
				break;

			if (!nextRes.valid)
				nextValidCount -= 1;
		}
		// 没气了，当前节点也无效 (Not valid, why ?)
		if (nextValidCount == 0)
			result.valid = false;

		return result;
	}

	float_t Path::CalcFarScore(vec2 player, vec2 target)
	{
		float_t score = _F(0.0);

		// 坐标原点移到左上角
		player += Scene::ORIGIN_POINT_OFFSET;
		target += Scene::ORIGIN_POINT_OFFSET;

		// 距离越远得分越高
		if (player.x < target.x)
			score += _F(0.5) * ((target.x - player.x) / target.x);
		else
			score += _F(0.5) * ((player.x - target.x) / (Scene::SIZE.x - target.x));

		if (player.y < target.y)
			score += _F(0.5) * ((target.y - player.y) / target.y);
		else
			score += _F(0.5) * ((player.y - target.y) / (Scene::SIZE.y - target.y));

		return score;
	}

	float_t Path::CalcNearScore(vec2 player, vec2 target)
	{
		float_t score = _F(0.0);

		// 坐标原点移到左上角
		player += Scene::ORIGIN_POINT_OFFSET;
		target += Scene::ORIGIN_POINT_OFFSET;

		// 距离越近得分越高
		if (player.x < target.x)
			score += _F(0.5) * (1 - (target.x - player.x) / target.x);
		else
			score += _F(0.5) * (1 - (player.x - target.x) / (Scene::SIZE.x - target.x));

		if (player.y < target.y)
			score += _F(0.5) * (1 - (target.y - player.y) / target.y);
		else
			score += _F(0.5) * (1 - (player.y - target.y) / (Scene::SIZE.y - target.y));

		return score;
	}

	float_t Path::CalcRelaxedNearScore(vec2 player, vec2 target, float32_t radius)
	{
		float_t score = _F(0.0);

		/*
		// 坐标原点移到左上角
		player += Scene::ORIGIN_POINT_OFFSET;
		target += Scene::ORIGIN_POINT_OFFSET;

		// 距离越近得分越高
		if (player.x < target.x)
			score += _F(0.5) * (1 - (target.x - player.x) / target.x);
		else
			score += _F(0.5) * (1 - (player.x - target.x) / (Scene::SIZE.x - target.x));

		if (player.y < target.y)
			score += _F(0.5) * (1 - (target.y - player.y) / target.y);
		else
			score += _F(0.5) * (1 - (player.y - target.y) / (Scene::SIZE.y - target.y));
		*/

		score += std::clamp(_F(1.0) - ((player - target).length() - radius) / Scene::SIZE.y, _F(0.0), _F(1.0));

		return score;
	}

	float_t Path::CalcShootScore(vec2 player, vec2 target)
	{
		float_t score = _F(0.0);

		// 坐标原点移到左上角
		player += Scene::ORIGIN_POINT_OFFSET;
		target += Scene::ORIGIN_POINT_OFFSET;

		// 距离越近得分越高
		if (player.x < target.x)
			score += _F(0.5) * (1 - (target.x - player.x) / target.x);
		else
			score += _F(0.5) * (1 - (player.x - target.x) / (Scene::SIZE.x - target.x));

		// 距离越远得分越高
		if (player.y < target.y)
			score += _F(-1.0);
		else
			score += _F(0.5) * ((player.y - target.y) / (Scene::SIZE.y - target.y));

		return score;
	}
}
