--[[
    This file is part of Ice Lua Components.

    Ice Lua Components is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Ice Lua Components is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Ice Lua Components.  If not, see <http://www.gnu.org/licenses/>.
]]

return function (plr)
	local this = {} this.this = this

	this.plr = plr
	this.mdl = mdl_nade_inst
	this.gui_x = 0.15
	this.gui_y = 0.25
	this.gui_scale = 0.1
	this.gui_pick_scale = 2.0
	this.t_nadeboom = nil
	this.t_newnade = nil
	this.ammo = 2

	function plr.expl_ammo_checkthrow()
		if this.ammo <= 0 then return false end

		if plr.mode == PLM_NORMAL then
			this.ammo = this.ammo - 1
		end

		return true
	end
	
	function this.restock()
		this.ammo = 4
	end
	
	function this.throw_nade(sec_current)
		local sya = math.sin(plr.angy)
		local cya = math.cos(plr.angy)
		local sxa = math.sin(plr.angx)
		local cxa = math.cos(plr.angx)
		local fwx,fwy,fwz
		fwx,fwy,fwz = sya*cxa, sxa, cya*cxa
		
		local n = new_nade({
			x = plr.x,
			y = plr.y,
			z = plr.z,
			vx = fwx*MODE_NADE_SPEED*MODE_NADE_STEP+plr.vx*MODE_NADE_STEP,
			vy = fwy*MODE_NADE_SPEED*MODE_NADE_STEP+plr.vy*MODE_NADE_STEP,
			vz = fwz*MODE_NADE_SPEED*MODE_NADE_STEP+plr.vz*MODE_NADE_STEP,
			fuse = math.max(0, this.t_nadeboom - sec_current),
			pid = this.plr.pid
		})
		nade_add(n)
		net_send(nil, common.net_pack("BhhhhhhH",
			PKT_NADE_THROW,
			math.floor(n.x*32+0.5),
			math.floor(n.y*32+0.5),
			math.floor(n.z*32+0.5),
			math.floor(n.vx*256+0.5),
			math.floor(n.vy*256+0.5),
			math.floor(n.vz*256+0.5),
			math.floor(n.fuse*100+0.5)))
	end
	
	function this.tick(sec_current, sec_delta)
		if this.t_newnade and sec_current >= this.t_newnade then
			this.t_newnade = nil
		end
		
		if this.t_nadeboom then
			if (not this.ev_lmb) or sec_current >= this.t_nadeboom then
				this.throw_nade(sec_current)
				this.t_newnade = sec_current + MODE_DELAY_NADE_THROW
				this.t_nadeboom = nil
				this.ev_lmb = false
			end
		end

		if client and plr.alive and (not plr.t_switch) then
		if this.ev_lmb and plr.mode ~= PLM_SPECTATE then
		if plr.tools[plr.tool+1] == this then
			if (not this.t_newnade) and this.ammo > 0 then
				if (not this.t_nadeboom) then
					if plr.mode == PLM_NORMAL then
						this.ammo = this.ammo - 1
					end
					this.t_nadeboom = sec_current + MODE_NADE_FUSE
					client.wav_play_global(wav_pin, plr.x, plr.y, plr.z)
					net_send(nil, common.net_pack("BhhhhhhH", PKT_NADE_PIN))
				end
			end
		end end end
	end
	
	function this.focus()
		--
	end
	
	function this.unfocus()
		--
	end

	function this.need_restock()
		return this.ammo < 4
	end

	function this.key(key, state, modif)
		--
	end

	function this.click(button, state)
		if button == 1 then
			-- LMB
			this.ev_lmb = state
		end
	end
	
	function this.free()
		--
	end

	function this.textgen()
		local col
		if this.ammo == 0 then
			col = 0xFFFF3232
		else
			col = 0xFFC0C0C0
		end
		return col, ""..this.ammo
	end
	
	function this.get_model()
		return this.mdl
	end
	
	function this.render(px, py, pz, ya, xa, ya2)
		if this.ammo > 0 then
			this.get_model().render_global(
				px, py, pz, ya, xa, ya2, 1)
		end
	end
	
	return this
end

