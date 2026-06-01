#pragma once

#include "Th10Ai/Common.h"

#include <optional>

#include "Th10Ai/Status.h"
#include "Th10Ai/Scene.h"

namespace th
{
	struct Action
	{
		vec2 fromPos;
		DIR fromDir;
		int_t frame;
		float_t score;
	};

	struct Result
	{
		bool valid;
		bool slow;		// 实际是否慢速(is_slow)
		float_t score;
	};

	class Path
	{
	public:
		static constexpr int_t FIND_LIMIT = 1000000000;
		static constexpr int_t FIND_DEPTH = 6;

		Path(Status& status, Scene* scenes,
			const std::optional<Item>& itemTarget,
			const std::optional<Bullet>& bulletTarget,
			const std::optional<Enemy>& enemyTarget,
			bool underEnemy, bool anyItems);

		Result find(DIR dir);
		Result findminmax(DIR dir);
		bool PlayerDies(const Action& action, Result& result, Player& player);
		void Update_status(Player& player, int_t i);
		float_t HorizonScore(Result& result, Player& player);
		float_t Score(Result& result, Player& player);
		Result minmax(const Action& action);
		Result dfs(const Action& action);

		std::optional<Item> findItem();
		std::optional<Enemy> findEnemy();
		std::optional<Bullet> findBullet();

	//private:
		static float_t CalcFarScore(vec2 player, vec2 target);
		static float_t CalcNearScore(vec2 player, vec2 target);
		static float_t CalcRelaxedNearScore(vec2 player, vec2 target, float32_t radius);
		static float_t CalcShootScore(vec2 player, vec2 target);

		static const DIR FIND_DIRS[to_underlying(DIR::MAX_COUNT)][5];
		static const int_t FIND_DIR_COUNTS[to_underlying(DIR::MAX_COUNT)];
		static const vec2 RESET_POS;

		Status& m_status;
		Scene* m_scenes;
		const std::optional<Item>& m_itemTarget;
		const std::optional<Enemy>& m_enemyTarget;
		const std::optional<Bullet>& m_bulletTarget;
		bool m_underEnemy;

		DIR m_dir;
		bool m_slowFirst;			// 是否慢速优先

		float_t m_bestScore;
		int_t m_count;

		bool m_anyItems;
	};
}
