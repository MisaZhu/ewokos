namespace std {
    void __throw_bad_function_call() {printf("%s\n", __func__);exit(-1);while(1);}
	void __throw_length_error(char const*) {printf("%s\n", __func__);exit(-1);while(1);}

	static void
	local_Rb_tree_rotate_left(_Rb_tree_node_base* const __x,
	  	             _Rb_tree_node_base*& __root)
	{
	  _Rb_tree_node_base* const __y = __x->_M_right;
	
	  __x->_M_right = __y->_M_left;
	  if (__y->_M_left !=0)
	    __y->_M_left->_M_parent = __x;
	  __y->_M_parent = __x->_M_parent;
	
	  if (__x == __root)
	    __root = __y;
	  else if (__x == __x->_M_parent->_M_left)
	    __x->_M_parent->_M_left = __y;
	  else
	    __x->_M_parent->_M_right = __y;
	  __y->_M_left = __x;
	  __x->_M_parent = __y;
	}

	static void
 	local_Rb_tree_rotate_right(_Rb_tree_node_base* const __x,
 	  		     _Rb_tree_node_base*& __root)
 	{
 	  _Rb_tree_node_base* const __y = __x->_M_left;

 	  __x->_M_left = __y->_M_right;
 	  if (__y->_M_right != 0)
 	    __y->_M_right->_M_parent = __x;
 	  __y->_M_parent = __x->_M_parent;

 	  if (__x == __root)
 	    __root = __y;
 	  else if (__x == __x->_M_parent->_M_right)
 	    __x->_M_parent->_M_right = __y;
 	  else
 	    __x->_M_parent->_M_left = __y;
 	  __y->_M_right = __x;
 	  __x->_M_parent = __y;
 	}

	static _Rb_tree_node_base*
	local_Rb_tree_decrement(_Rb_tree_node_base* __x)
	{
	  if (__x->_M_color == _S_red
	      && __x->_M_parent->_M_parent == __x)
	    __x = __x->_M_right;
	  else if (__x->_M_left != 0)
	    {
	      _Rb_tree_node_base* __y = __x->_M_left;
	      while (__y->_M_right != 0)
	        __y = __y->_M_right;
	      __x = __y;
	    }
	  else
	    {
	      _Rb_tree_node_base* __y = __x->_M_parent;
	      while (__x == __y->_M_left)
	        {
	          __x = __y;
	          __y = __y->_M_parent;
	        }
	      __x = __y;
	    }
	  return __x;
	}

	_Rb_tree_node_base* _Rb_tree_decrement(_Rb_tree_node_base* __x)
	{
	  return local_Rb_tree_decrement(__x);
	}

	 void _Rb_tree_insert_and_rebalance(const bool          __insert_left,
                                _Rb_tree_node_base* __x,
                                _Rb_tree_node_base* __p,
                                _Rb_tree_node_base& __header)
	{
	  
		 _Rb_tree_node_base *& __root = __header._M_parent;
	
	 	 // Initialize fields in new node to insert.
	 	 __x->_M_parent = __p;
	 	 __x->_M_left = 0;
	 	 __x->_M_right = 0;
	 	 __x->_M_color = _S_red;
	
	 	 // Insert.
	 	 // Make new node child of parent and maintain root, leftmost and
	 	 // rightmost nodes.
	 	 // N.B. First node is always inserted left.
	 	 if (__insert_left)
	 	   {
	 	     __p->_M_left = __x; // also makes leftmost = __x when __p == &__header
	
	 	     if (__p == &__header)
	 	     {
	 	         __header._M_parent = __x;
	 	         __header._M_right = __x;
	 	     }
	 	     else if (__p == __header._M_left)
	 	       __header._M_left = __x; // maintain leftmost pointing to min node
	 	   }
	 	 else
	 	   {
	 	     __p->_M_right = __x;
	
	 	     if (__p == __header._M_right)
	 	       __header._M_right = __x; // maintain rightmost pointing to max node
	 	   }
	 	 // Rebalance.
	 	 while (__x != __root
	 	    && __x->_M_parent->_M_color == _S_red)
	 	   {
	 	 _Rb_tree_node_base* const __xpp = __x->_M_parent->_M_parent;
	
	 	 if (__x->_M_parent == __xpp->_M_left)
	 	   {
	 	     _Rb_tree_node_base* const __y = __xpp->_M_right;
	 	     if (__y && __y->_M_color == _S_red)
	 	       {
	 	 	__x->_M_parent->_M_color = _S_black;
	 	 	__y->_M_color = _S_black;
	 	 	__xpp->_M_color = _S_red;
	 	 	__x = __xpp;
	 	       }
	 	     else
	 	       {
	 	 	if (__x == __x->_M_parent->_M_right)
	 	 	  {
	 	 	    __x = __x->_M_parent;
	 	 	    local_Rb_tree_rotate_left(__x, __root);
	 	 	  }
	 	 	__x->_M_parent->_M_color = _S_black;
	 	 	__xpp->_M_color = _S_red;
	 	 	local_Rb_tree_rotate_right(__xpp, __root);
	 	       }
	 	   }
	 	 else
	 	   {
	 	     _Rb_tree_node_base* const __y = __xpp->_M_left;
	 	     if (__y && __y->_M_color == _S_red)
	 	       {
	 	 	__x->_M_parent->_M_color = _S_black;
	 	 	__y->_M_color = _S_black;
	 	 	__xpp->_M_color = _S_red;
	 	 	__x = __xpp;
	 	       }
	 	     else
	 	       {
	 	 	if (__x == __x->_M_parent->_M_left)
	 	 	  {
	 	 	    __x = __x->_M_parent;
	 	 	    local_Rb_tree_rotate_right(__x, __root);
	 	 	  }
	 	 	__x->_M_parent->_M_color = _S_black;
	 	 	__xpp->_M_color = _S_red;
	 	 	local_Rb_tree_rotate_left(__xpp, __root);
	 	       }
	 	   }
	 	   }
	 	 __root->_M_color = _S_black;
	}
}

