/*
    This file is part of Iceball.

    Iceball is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Iceball is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Iceball.  If not, see <http://www.gnu.org/licenses/>.
*/

int icelua_fn_client_draw_flush(lua_State *L)
{
	draw_flush();
	return 0;
}

int icelua_fn_client_draw_texture(lua_State *L)
{
	int top = icelua_assert_stack(L, 10, 10);
	
	if(lua_islightuserdata(L, 1) || !lua_isuserdata(L, 1))
		return luaL_error(L, "not an image");
	img_t *img = (img_t *)lua_touserdata(L, 1);
	if(img == NULL || img->udtype != UD_IMG)
		return luaL_error(L, "not an image");

	int iw, ih;
	iw = img->head.width;
	ih = img->head.height;
	expandtex_gl(&iw, &ih);
	if(img->tex == 0)
	{
		glEnable(GL_TEXTURE_2D);
		glGenTextures(1, &img->tex);
		glBindTexture(GL_TEXTURE_2D, img->tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, iw, ih, 0, GL_BGRA, GL_UNSIGNED_BYTE, img->pixels);
		img->tex_dirty = 0;
	}

	float x = lua_tonumber(L, 2);
	float y = lua_tonumber(L, 3);
	float w = lua_tonumber(L, 4);
	float h = lua_tonumber(L, 5);
	float s = lua_tonumber(L, 6);
	float t = lua_tonumber(L, 7);
	float u = lua_tonumber(L, 8);
	float v = lua_tonumber(L, 9);
	uint32_t color = lua_tointeger(L, 10);

	draw_quad(x, y, w, h, s / (float)iw, t / (float)ih, u / (float)iw, v / (float)ih, img->tex, color);

	return 0;
}

int icelua_fn_client_draw_quad(lua_State *L)
{
	int top = icelua_assert_stack(L, 5, 10);

	float x = lua_tonumber(L, 1);
	float y = lua_tonumber(L, 2);
	float w = lua_tonumber(L, 3);
	float h = lua_tonumber(L, 4);
	uint32_t color = lua_tointeger(L, 5);

	GLuint texture = 0;
	if (top == 10) {
		float s = lua_tonumber(L, 6);
		float t = lua_tonumber(L, 7);
		float u = lua_tonumber(L, 8);
		float v = lua_tonumber(L, 9);
		if(lua_islightuserdata(L, 1) || !lua_isuserdata(L, 1))
			return luaL_error(L, "not an image");
		img_t *img = (img_t *)lua_touserdata(L, 1);
		if(img == NULL || img->udtype != UD_IMG)
			return luaL_error(L, "not an image");

		draw_quad(x, y, w, h, s, t, u, v, img->tex, color);
	} else {
		draw_blank_quad(x, y, w, h, color);
	}

	return 0;
}
