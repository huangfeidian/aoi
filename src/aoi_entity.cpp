#include "aoi_entity.h"
#include "set_utility.h"

bool aoi_entity::pos_in_aoi_radius(const aoi_entity& other) const
{
    auto diff_x = m_pos[0] - other.m_pos[0];
    auto diff_y = m_pos[1] - other.m_pos[1];
    auto diff_z = m_pos[2] - other.m_pos[2];
    if((diff_x * diff_x + diff_z * diff_z) > m_aoi_ctrl.radius * m_aoi_ctrl.radius)
    {
        return false;
    }
    if(m_aoi_ctrl.height)
    {
        if(diff_y > m_aoi_ctrl.height || diff_y < -m_aoi_ctrl.height)
        {
            return false;
        }
    }
    return true;
}
bool aoi_entity::can_pass_flag_check(const aoi_entity& other) const
{
    // other能否进入当前entity的aoi 的flag测试
    if(other.m_guid == m_guid)
    {
        return false;
    }
    if(has_flag(aoi_flag::forbid_interest_in))
    {
        return false;
    }
    if(other.has_flag(aoi_flag::forbid_interested_by))
    {
        return false;
    }
    return m_aoi_ctrl.interest_in_flag & other.m_aoi_ctrl.interested_by_flag;// 如果两个flag取and 不为0 则代表可以进入aoi
}
bool aoi_entity::can_add_enter(const aoi_entity& other, bool ignore_dist, bool force_add) const
{
    if(!can_pass_flag_check(other))
    {
        return false;
    }
    if(!force_add && other.has_flag(aoi_flag::limited_by_max) && m_interest_in.size() >= m_aoi_ctrl.max_interest_in)
    {
        return false;
    }
    if (has_interest_in(other.aoi_idx))
    {
        return false;
    }
    if(!ignore_dist)
    {
        if(!pos_in_aoi_radius(other))
        {
            return false;
        }
    }
    return true;
}
bool aoi_entity::enter_by_pos(aoi_entity& other, bool ignore_dist)
{
    if(!can_add_enter(other, ignore_dist))
    {
        return false;
    }
    enter_impl(other);
    return true;
}
void aoi_entity::enter_impl(aoi_entity& other)
{		

    set_interest_in(other.aoi_idx);
    other.set_interested_by(this->aoi_idx);
    m_aoi_callback(m_guid, other.m_guid, true);
}

bool aoi_entity::enter_by_force(aoi_entity& other)
{
    if (std::find(m_force_interest_in.begin(), m_force_interest_in.end(), other.aoi_idx) != m_force_interest_in.end())
    {
        return false;
    }
    if(!can_add_enter(other, true, true))
    {
        return false;
    }
    

    m_force_interest_in.push_back(other.aoi_idx);
    other.m_force_interested_by.push_back(aoi_idx);
    enter_impl(other);
    return true;
    
}
void aoi_entity::leave_impl(aoi_entity& other)
{
    remove_interest_in(other.aoi_idx);
    other.remove_interested_by(this->aoi_idx);
    m_aoi_callback(m_guid, other.m_guid, false);
}
bool aoi_entity::leave_by_pos(aoi_entity& other)
{
    if(!has_interest_in(other.aoi_idx))
    {
        return false;
    }
    if (std::find(m_force_interest_in.begin(), m_force_interest_in.end(), other.aoi_idx) != m_force_interest_in.end())
    {
        return false;
    }
    leave_impl(other);
    return true;
}
bool aoi_entity::leave_by_force(aoi_entity& other)
{
    if (!remove_in_vec(m_force_interest_in, other.aoi_idx))
    {
        return false;
    }
    
    if(!pos_in_aoi_radius(other))
    {
        leave_impl(other);
        return true;
    }
    else
    {
        return false;
    }
}

void aoi_entity::activate(guid_t in_guid, const aoi_controler& aoi_ctrl, const pos_t& in_pos, const aoi_callback_t aoi_cb)
{
    m_guid = in_guid;
    m_aoi_ctrl = aoi_ctrl;
    m_aoi_callback = aoi_cb;
    m_pos = in_pos;
    cacl_data = nullptr;
}

void aoi_entity::deactivate()
{
    m_aoi_callback = nullptr;
    m_guid = 0;
    cacl_data = nullptr;
}

void aoi_entity::update_by_pos(const std::vector<aoi_entity*>& new_interest_in, const std::vector<aoi_entity*>& all_entities, std::vector<std::uint8_t>& diff_vec)
{
    for (auto one_ent : new_interest_in)
    {
        diff_vec[one_ent->aoi_idx] = 1;
    }
    for (auto one_ent_idx : m_interest_in)
    {
        if (!diff_vec[one_ent_idx])
        {
            leave_by_pos(*all_entities[one_ent_idx]);
        }
    }
    for (auto one_ent : new_interest_in)
    {
        enter_by_pos(*one_ent, true);
        diff_vec[one_ent->aoi_idx] = 0;
    }
}

bool aoi_entity::try_enter(aoi_entity& other)
{
    return enter_by_pos(other);
}

bool aoi_entity::try_leave(aoi_entity& other)
{
    if (!pos_in_aoi_radius(other))
    {
        return leave_by_pos(other);
    }
    return false;
}