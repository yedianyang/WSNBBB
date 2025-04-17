Axis::Axis(INode *node) : 
	m_node(node), 
	m_moveCount(0), 
	m_positionWrapCount(0),
	m_quitting(false),
	m_monitoring(false) {
} 