#include "aoi_entity.h"
#include "frozen_pool.h"

#include <unordered_map>

class aoi_caclator;

class aoi_manager
{
public:
	aoi_manager(aoi_caclator* aoi_impl, std::uint32_t max_entity_size, pos_t min, pos_t max);
	~aoi_manager();

	bool add_entity(guid_t guid, std::uint16_t radius, std::uint16_t height, pos_t pos, std::uint32_t flag, std::uint16_t max_interested);
	bool remove_entity(guid_t guid);
	bool change_entity_radius(guid_t guid, std::uint16_t radius);
	bool change_entity_pos(guid_t guid, pos_t pos);
	// 将guid_from 加入到guid_to的强制关注列表里
	bool add_force_aoi(guid_t guid_from, guid_t guid_to);
	// 将guid_from 移除出guid_to 的强制关注列表
	bool remove_force_aoi(guid_t guid_from, guid_t guid_to);
	const std::vector<guid_t>& interest_in(guid_t guid)const;
	const std::vector<guid_t>& interested_by(guid_t guid) const;
	// 更新内部信息 返回所有有callback消息通知的entity guid 列表
	// 只有在调用update之后 所有entity的aoi interested 才是最新的正确的值
	std::vector<guid_t> update();
public:
	// 获取平面区域内圆形范围内的entity列表
	std::vector<guid_t> entity_in_circle(pos_t center, std::uint16_t radius) const;
	// 获取平面区域内圆柱形区域内的entity列表
	std::vector<guid_t> entity_in_cylinder(pos_t center, std::uint16_t radius, std::uint16_t height) const;
	// 获取 平面内矩形区域内的entity列表
	std::vector<guid_t> entity_in_rectangle(pos_t center, std::uint16_t x_width, std::uint16_t z_width) const;
	// 获取平面内正方形区域内的entity 列表
	std::vector<guid_t> entity_in_square(pos_t center, std::uint16_t width) const;
	// 获取长方体区域内的entity列表
	std::vector<guid_t> entity_in_cuboid(pos_t center, std::uint16_t x_width, std::uint16_t z_width, std::uint16_t y_height) const;
	// 获取立方体区域内的entity列表
	std::vector<guid_t> entity_in_cube(pos_t center, std::uint16_t width) const;
	// 获取2d 扇形区域内的entity 列表 yaw为扇形中心朝向的弧度， yaw range为左右两侧的弧度偏移
	std::vector<guid_t> entity_in_fan(pos_t center, std::uint16_t radius, float yaw, float yaw_range) const;
private:
	frozen_pool<aoi_entity> entity_pool;
	std::unordered_map<guid_t, aoi_entity*> all_guids;
	aoi_caclator* aoi_impl;
	pos_t min;
	pos_t max;
	std::vector<guid_t> invalid_set;// for invalid guid

}