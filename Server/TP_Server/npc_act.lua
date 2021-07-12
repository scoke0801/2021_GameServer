myid = -1  
mydir = -1
move_count = 0

WORLD_WIDTH = 2000
WORLD_HEIGHT = 2000
litsnter = -1
function set_uid(x)
	myid = x
end   

function run_away_from_player(p_id)   
	m_x = API_get_x(myid)
	m_y = API_get_y(myid)

	if (move_count == 0) then
		mydir = math.random(4)
		API_set_move_type(myid, 1)
	end

	if(mydir == 1) then
		if(m_x < WORLD_WIDTH) then
			API_set_x(myid, m_x + 1)
		end
	end
	if(mydir == 2) then
		if(m_x > 0) then
			API_set_x(myid, m_x - 1)
		end
	end
	if(mydir == 3) then
		if(m_y < WORLD_HEIGHT) then
			API_set_y(myid, m_y + 1)
		end
	end
	if(mydir == 4) then
		if(m_y > 0) then
			API_set_y(myid, m_y - 1)
		end
	end

	if (move_count == 7) then 
		move_count = 0
		mydir = 0
		API_set_move_type(myid, 0)
		API_add_event(myid, 1000, 4)
		API_send_mess(p_id, myid, "BYE")
	else 
		move_count = move_count + 1
	end 
end