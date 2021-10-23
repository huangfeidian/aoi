#include "aoi_entity.h"
#include "set_utility.h"

bool aoi_entity::pos_in_aoi_radius(const aoi_entity& other) const
{
    auto diff_x = pos[0] - other.pos[0];
    auto diff_y = pos[1] - other.pos[1];
    auto diff_z = pos[2] - other.pos[2];
    if((diff_x * diff_x + diff_z * diff_z) > radius * radius)
    {
        return false;
    }
    if(height)
    {
        if(diff_y > height || diff_y < -height)
        {
            return false;
        }
    }
    return true;
}
bool aoi_entity::can_pass_flag_check(const aoi_entity& other) const
{
    if(other.guid == guid)
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
    return true;
}
bool aoi_entity::can_add_enter(const aoi_entity& other, bool ignore_dist, bool force_add) const
{
    if(!can_pass_flag_check(other))
    {
        return false;
    }
    if(!force_add && other.has_flag(aoi_flag::limited_by_max) && interest_in.size() >= max_interest_in)
    {
        return false;
    }
    if(interest_in.find(other.guid) != interest_in.end())
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
    interest_in.insert(other.guid);
    other.interested_by.insert(guid);
}

bool aoi_entity::enter_by_force(aoi_entity& other)
{
    if(!can_add_enter(other, true, true))
    {
        return false;
    }
    if(force_interest_in.find(other.guid) != force_interest_in.end())
    {
        return false;
    }
    force_interest_in.insert(other.guid);
    other.force_interested_by.insert(guid);
    enter_impl(other);
    return true;
}
void aoi_entity::leave_impl(aoi_entity& other)
{
    interest_in.erase(other.guid);
    other.interested_by.erase(guid);
}
bool aoi_entity::leave_by_pos(aoi_entity& other)
{
    if(interest_in.find(other.guid) == interest_in.end())
    {
        return false;
    }
    if(force_interest_in.find(other.guid) != force_interest_in.end())
    {
        return false;
    }
    leave_impl(other);
    return true;
}
bool aoi_entity::leave_by_force(aoi_entity& other)
{
    if(force_interest_in.erase(other.guid) != 1)
    {
        return false;
    }
    other.force_interested_by.erase(guid);
    
    if(interest_in.find(other.guid) == interest_in.end())
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
void aoi_entity::after_update(std::unordered_set<guid_t>& enter_guids, std::unordered_set<guid_t>& leave_guids)
{
    // 处理完成之后 更新之前的记录 
    std::unordered_set<guid_t> temp_pre_interest_in{prev_interest_in.begin(), prev_interest_in.end()};
    unordered_set_diff(interest_in, temp_pre_interest_in, enter_guids);
    unordered_set_diff(temp_pre_interest_in, interest_in, leave_guids);
    prev_interest_in.clear();
    prev_interested_by.clear();
    prev_interest_in.insert(prev_interest_in.end(), interest_in.begin(), interest_in.end());
    prev_interested_by.insert(prev_interested_by.end(), interested_by.begin(), interested_by.end());
}
