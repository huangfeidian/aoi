#include "aoi_entity.h"
#include "frozen_pool.h"

#include <unordered_map>
#include <functional>
#include <ostream>

class aoi_interface;
using aoi_callback_t = std::function<void(guid_t, const std::vector<guid_t>&, const std::vector<guid_t>&)>;
class aoi_manager
{
public:
	aoi_manager(aoi_interface* aoi_impl, std::uint32_t max_entity_size, pos_unit_t max_aoi_radius, pos_t min, pos_t max);
	~aoi_manager();

	bool add_entity(guid_t guid, pos_unit_t radius, pos_unit_t height, pos_t pos, std::uint32_t flag, std::uint16_t max_interested);
	bool remove_entity(guid_t guid);
	bool change_entity_radius(guid_t guid, pos_unit_t radius);
	bool change_entity_pos(guid_t guid, pos_t pos);
	// 将guid_from 加入到guid_to的强制关注列表里
	bool add_force_aoi(guid_t guid_from, guid_t guid_to);
	// 将guid_from 移除出guid_to 的强制关注列表
	bool remove_force_aoi(guid_t guid_from, guid_t guid_to);
	const std::unordered_set<guid_t>& interest_in(guid_t guid)const;
	const std::unordered_set<guid_t>& interested_by(guid_t guid) const;
	// 更新内部信息 返回所有有callback消息通知的entity guid 列表
	// 只有在调用update之后 所有entity的aoi interested 才是最新的正确的值
	// T 的类型应该能转换到aoi_callback_t 这里用模板是为了避免直接用aoi_callback_t时引发的虚函数开销
	template <typename T>
	std::vector<guid_t> update(T aoi_callback)
	{
		std::unordered_set<aoi_entity*> temp_result;
		unordered_set_union(aoi_impl->update_all(), entities_removed, temp_result);


		std::vector<guid_t> result;
		result.reserve(temp_result.size());
		std::unordered_set<guid_t> enter_guids;
		std::unordered_set<guid_t> leave_guids;
		for (auto one_entity : temp_result)
		{
			result.push_back(one_entity->guid);
			enter_guids.clear();
			leave_guids.clear();
			one_entity->after_update(enter_guids, leave_guids);
			aoi_callback(one_entity->guid, enter_guids, leave_guids);
		}
		temp_result = entities_removed;
		for (auto one_entity : temp_result)
		{
			all_guids.erase(one_entity->guid);
		}
		entities_removed.clear();

		return result;
	}
	bool is_in_border(const pos_t& pos) const;
public:
	// 获取平面区域内圆形范围内的entity列表
	std::vector<guid_t> entity_in_circle(pos_t center, pos_unit_t radius);
	// 获取平面区域内圆柱形区域内的entity列表
	std::vector<guid_t> entity_in_cylinder(pos_t center, pos_unit_t radius, pos_unit_t height);
	// 获取 平面内矩形区域内的entity列表
	std::vector<guid_t> entity_in_rectangle(pos_t center, pos_unit_t x_width, pos_unit_t z_width);
	// 获取平面内正方形区域内的entity 列表
	std::vector<guid_t> entity_in_square(pos_t center, pos_unit_t width);
	// 获取长方体区域内的entity列表
	std::vector<guid_t> entity_in_cuboid(pos_t center, pos_unit_t x_width, pos_unit_t z_width, pos_unit_t y_height);
	// 获取立方体区域内的entity列表
	std::vector<guid_t> entity_in_cube(pos_t center, pos_unit_t width);
	// 获取2d 扇形区域内的entity 列表 yaw为扇形中心朝向的弧度， yaw range为左右两侧的弧度偏移
	std::vector<guid_t> entity_in_fan(pos_t center, pos_unit_t radius, float yaw, float yaw_range);
	void dump(std::ostream& out_debug) const;
private:
	frozen_pool<aoi_entity> entity_pool;
	std::unordered_set<aoi_entity*> entities_removed;
	std::unordered_map<guid_t, aoi_entity*> all_guids;
	aoi_interface* aoi_impl;
	pos_t min;
	pos_t max;
	pos_unit_t max_aoi_radius;
	std::uint32_t max_entity_size;
	std::unordered_set<guid_t> invalid_result;// for invalid guid

};