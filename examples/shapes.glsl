float circle( in vec2 _st, in float _size ){
	if( length(_st) < _size){
		return 1.0;
	}
	return 0.0;
}

float circle( in vec2 _st, in float _size, in float _lineWidth ){	
	float dist = length(_st);
	if( dist < _size && 
		dist > _size - _lineWidth){
		return 1.0;
	}
	return 0.0;
}

float box( in vec2 _st, in float _size ){
	if( _st.x + _size > 0.0 &&
		_st.x - _size < 0.0 &&
		_st.y + _size > 0.0 &&
		_st.y - _size < 0.0){
		return 1.0;
	}
	return 0.0;
}

float box( in vec2 _st, in float _size, in float _lineWidth ){
	if( box(_st,_size) + box(_st,_size-_lineWidth) == 1.0 ){
		return 1.0;
	}
	return 0.0;
}

float grid(in vec2 _st, in float _size, in float _lineWidth){
	vec2 grid = fract(_st*_size);
	if( grid.x < _lineWidth || 
		grid.y < _lineWidth ){
		return 1.0;
	}
	return 0.0;
}

float cross( in vec2 _st, in float _size, in float _lineWidth ){
	if(box(_st,_size) == 1.0){
		if( abs(_st.x) < _lineWidth || 
			abs(_st.y) < _lineWidth+0.00005 ){
			return 1.0;
		}
	}
	return 0.0;
}

