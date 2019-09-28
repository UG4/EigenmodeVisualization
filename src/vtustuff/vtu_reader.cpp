
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//	implementation of GridReaderUGX
GridReaderUGX::GridReaderUGX()
{
}

GridReaderUGX::~GridReaderUGX()
{
}

const char* GridReaderUGX::
get_grid_name(size_t index) const
{
	assert(index < num_grids() && "Bad index!");
	xml_attribute<>* attrib = m_entries[index].node->first_attribute("name");
	if(attrib)
		return attrib->value();
	return "";
}

size_t GridReaderUGX::num_subset_handlers(size_t refGridIndex) const
{
//	access the referred grid-entry
	if(refGridIndex >= m_entries.size()){
		UG_LOG("GridReaderUGX::num_subset_handlers: bad refGridIndex. Aborting.\n");
		return 0;
	}

	return m_entries[refGridIndex].subsetHandlerEntries.size();
}

const char* GridReaderUGX::
get_subset_handler_name(size_t refGridIndex, size_t subsetHandlerIndex) const
{
	assert(refGridIndex < num_grids() && "Bad refGridIndex!");
	const GridEntry& ge = m_entries[refGridIndex];
	assert(subsetHandlerIndex < ge.subsetHandlerEntries.size() && "Bad subsetHandlerIndex!");

	xml_attribute<>* attrib = ge.subsetHandlerEntries[subsetHandlerIndex].node->first_attribute("name");
	if(attrib)
		return attrib->value();
	return "";
}

bool GridReaderUGX::
subset_handler(ISubsetHandler& shOut,
					size_t subsetHandlerIndex,
					size_t refGridIndex)
{
//	access the referred grid-entry
	if(refGridIndex >= m_entries.size()){
		UG_LOG("GridReaderUGX::subset_handler: bad refGridIndex. Aborting.\n");
		return false;
	}

	GridEntry& gridEntry = m_entries[refGridIndex];

//	get the referenced subset-handler entry
	if(subsetHandlerIndex >= gridEntry.subsetHandlerEntries.size()){
		UG_LOG("GridReaderUGX::subset_handler: bad subsetHandlerIndex. Aborting.\n");
		return false;
	}

	SubsetHandlerEntry& shEntry = gridEntry.subsetHandlerEntries[subsetHandlerIndex];
	shEntry.sh = &shOut;

	xml_node<>* subsetNode = shEntry.node->first_node("subset");
	size_t subsetInd = 0;
	while(subsetNode)
	{
	//	set subset info
	//	retrieve an initial subset-info from shOut, so that initialised values are kept.
		SubsetInfo si = shOut.subset_info(subsetInd);

		xml_attribute<>* attrib = subsetNode->first_attribute("name");
		if(attrib)
			si.name = attrib->value();

		attrib = subsetNode->first_attribute("color");
		if(attrib){
			stringstream ss(attrib->value(), ios_base::in);
			for(size_t i = 0; i < 4; ++i)
				ss >> si.color[i];
		}

		attrib = subsetNode->first_attribute("state");
		if(attrib){
			stringstream ss(attrib->value(), ios_base::in);
			size_t state;
			ss >> state;
			si.subsetState = (uint)state;
		}

		shOut.set_subset_info(subsetInd, si);

	//	read elements of this subset
		if(shOut.elements_are_supported(SHE_VERTEX))
			read_subset_handler_elements<Vertex>(shOut, "vertices",
													 subsetNode, subsetInd,
													 gridEntry.vertices);
		if(shOut.elements_are_supported(SHE_EDGE))
			read_subset_handler_elements<Edge>(shOut, "edges",
													 subsetNode, subsetInd,
													 gridEntry.edges);
		if(shOut.elements_are_supported(SHE_FACE))
			read_subset_handler_elements<Face>(shOut, "faces",
												 subsetNode, subsetInd,
												 gridEntry.faces);
		if(shOut.elements_are_supported(SHE_VOLUME))
			read_subset_handler_elements<Volume>(shOut, "volumes",
												 subsetNode, subsetInd,
												 gridEntry.volumes);
	//	next subset
		subsetNode = subsetNode->next_sibling("subset");
		++subsetInd;
	}

	return true;
}

template <class TGeomObj>
bool GridReaderUGX::
read_subset_handler_elements(ISubsetHandler& shOut,
							 const char* elemNodeName,
							 rapidxml::xml_node<>* subsetNode,
							 int subsetIndex,
							 std::vector<TGeomObj*>& vElems)
{
	xml_node<>* elemNode = subsetNode->first_node(elemNodeName);

	while(elemNode)
	{
	//	read the indices
		stringstream ss(elemNode->value(), ios_base::in);

		size_t index;
		while(!ss.eof()){
			ss >> index;
			if(ss.fail())
				continue;

			if(index < vElems.size()){
				shOut.assign_subset(vElems[index], subsetIndex);
			}
			else{
				UG_LOG("Bad element index in subset-node " << elemNodeName <<
						": " << index << ". Ignoring element.\n");
				return false;
			}
		}

	//	get next element node
		elemNode = elemNode->next_sibling(elemNodeName);
	}

	return true;
}


///	returns the number of selectors for the given grid
size_t GridReaderUGX::
num_selectors(size_t refGridIndex) const
{
	UG_COND_THROW(refGridIndex >= m_entries.size(),
				  "Bad refGridIndex: " << refGridIndex);

	return m_entries[refGridIndex].selectorEntries.size();
}

///	returns the name of the given selector
const char* GridReaderUGX::
get_selector_name(size_t refGridIndex, size_t selectorIndex) const
{
	UG_COND_THROW(refGridIndex >= m_entries.size(),
				  "Bad refGridIndex: " << refGridIndex);
	const GridEntry& ge = m_entries[refGridIndex];
	assert(selectorIndex < ge.selectorEntries.size() && "Bad selectorIndex!");

	xml_attribute<>* attrib = ge.selectorEntries[selectorIndex].node->first_attribute("name");
	if(attrib)
		return attrib->value();
	return "";
}

///	fills the given selector
bool GridReaderUGX::
selector(ISelector& selOut, size_t selectorIndex, size_t refGridIndex)
{
//	access the referred grid-entry
	if(refGridIndex >= m_entries.size()){
		UG_LOG("GridReaderUGX::selector: bad refGridIndex. Aborting.\n");
		return false;
	}

	GridEntry& gridEntry = m_entries[refGridIndex];

//	get the referenced subset-handler entry
	if(selectorIndex >= gridEntry.selectorEntries.size()){
		UG_LOG("GridReaderUGX::selector: bad selectorIndex. Aborting.\n");
		return false;
	}

	SelectorEntry& selEntry = gridEntry.selectorEntries[selectorIndex];
	selEntry.sel = &selOut;

	xml_node<>* selectorNode = selEntry.node;

//	read elements of this subset
	if(selOut.elements_are_supported(SHE_VERTEX))
		read_selector_elements<Vertex>(selOut, "vertices",
											selectorNode,
											gridEntry.vertices);
	if(selOut.elements_are_supported(SHE_EDGE))
		read_selector_elements<Edge>(selOut, "edges",
											selectorNode,
											gridEntry.edges);
	if(selOut.elements_are_supported(SHE_FACE))
		read_selector_elements<Face>(selOut, "faces",
										selectorNode,
										gridEntry.faces);
	if(selOut.elements_are_supported(SHE_VOLUME))
		read_selector_elements<Volume>(selOut, "volumes",
										selectorNode,
										gridEntry.volumes);

	return true;
}

template <class TGeomObj>
bool GridReaderUGX::
read_selector_elements(ISelector& selOut, const char* elemNodeName,
				   	   rapidxml::xml_node<>* selNode,
				   	   std::vector<TGeomObj*>& vElems)
{
	xml_node<>* elemNode = selNode->first_node(elemNodeName);

	while(elemNode)
	{
	//	read the indices
		stringstream ss(elemNode->value(), ios_base::in);

		size_t index;
		int state;

		while(!ss.eof()){
			ss >> index;
			if(ss.fail())
				continue;

			ss >> state;
			if(ss.fail())
				continue;

			if(index < vElems.size()){
				selOut.select(vElems[index], state);
			}
			else{
				UG_LOG("Bad element index in subset-node " << elemNodeName <<
						": " << index << ". Ignoring element.\n");
				return false;
			}
		}

	//	get next element node
		elemNode = elemNode->next_sibling(elemNodeName);
	}
	return true;
}


size_t GridReaderUGX::
num_projection_handlers(size_t refGridIndex) const
{
	UG_COND_THROW(refGridIndex >= m_entries.size(),
				  "Bad refGridIndex: " << refGridIndex);

	return m_entries[refGridIndex].projectionHandlerEntries.size();
}

const char* GridReaderUGX::
get_projection_handler_name(size_t refGridIndex, size_t phIndex) const
{
	UG_COND_THROW(refGridIndex >= m_entries.size(),
			  "Bad refGridIndex: " << refGridIndex);
	
	const GridEntry& ge = m_entries[refGridIndex];
	UG_COND_THROW(phIndex >= ge.projectionHandlerEntries.size(),
			  "Bad projection-handler-index: " << phIndex);

	xml_attribute<>* attrib = ge.projectionHandlerEntries[phIndex]->first_attribute("name");
	if(attrib)
		return attrib->value();
	return "";
}


size_t GridReaderUGX::get_projection_handler_subset_handler_index(size_t phIndex, size_t refGridIndex)
{
	UG_COND_THROW(refGridIndex >= m_entries.size(),
			  "Bad refGridIndex: " << refGridIndex);

	const GridEntry& ge = m_entries[refGridIndex];
	UG_COND_THROW(phIndex >= ge.projectionHandlerEntries.size(),
			  "Bad projection-handler-index: " << phIndex);

	xml_node<>* phNode = ge.projectionHandlerEntries[phIndex];

	size_t shi = 0;
	xml_attribute<>* attribSH = phNode->first_attribute("subset_handler");
	if (attribSH) shi = atoi(attribSH->value());

	return shi;
}


SPRefinementProjector GridReaderUGX::
read_projector(xml_node<>* projNode)
{
	static Factory<RefinementProjector, ProjectorTypes>	projFac;
	static Archivar<boost::archive::text_iarchive, RefinementProjector, ProjectorTypes>	archivar;

	xml_attribute<>* attribType = projNode->first_attribute("type");
	if(attribType){
		try {
			SPRefinementProjector proj = projFac.create(attribType->value());

			string str(projNode->value(), projNode->value_size());
			stringstream ss(str, ios_base::in);
			boost::archive::text_iarchive ar(ss, boost::archive::no_header);
			archivar.archive(ar, *proj);
			return proj;
		}
		catch(boost::archive::archive_exception e){
			UG_LOG("WARNING: Couldn't read projector of type '" <<
					attribType->value() << "'." << std::endl);
		}
	}
	return SPRefinementProjector();
}

bool GridReaderUGX::
projection_handler(ProjectionHandler& phOut, size_t phIndex, size_t refGridIndex)
{
	UG_COND_THROW(refGridIndex >= m_entries.size(),
			  "Bad refGridIndex: " << refGridIndex);
	
	const GridEntry& ge = m_entries[refGridIndex];
	UG_COND_THROW(phIndex >= ge.projectionHandlerEntries.size(),
			  "Bad projection-handler-index: " << phIndex);

	xml_node<>* phNode = ge.projectionHandlerEntries[phIndex];
	
	xml_node<>* defProjNode = phNode->first_node("default");
	if(defProjNode){
		SPRefinementProjector proj = read_projector(defProjNode);
		if(proj.valid()){
			phOut.set_default_projector(proj);
		}
	}

	xml_node<>* projNode = phNode->first_node("projector");
	while(projNode){
		SPRefinementProjector proj = read_projector(projNode);
		if(!proj.valid())
			continue;

		xml_attribute<>* attribSI = projNode->first_attribute("subset");
		if(attribSI){
			phOut.set_projector(atoi(attribSI->value()), proj);
		}

		projNode = projNode->next_sibling("projector");
	}

	return true;
}


bool GridReaderUGX::
parse_file(const char* filename)
{
	ifstream in(filename, ios::binary);
	if(!in)
		return false;

//	get the length of the file
	streampos posStart = in.tellg();
	in.seekg(0, ios_base::end);
	streampos posEnd = in.tellg();
	streamsize size = posEnd - posStart;

//	go back to the start of the file
	in.seekg(posStart);

//	read the whole file en-block and terminate it with 0
	char* fileContent = m_doc.allocate_string(0, size + 1);
	in.read(fileContent, size);
	fileContent[size] = 0;
	in.close();

//	parse the xml-data
	m_doc.parse<0>(fileContent);

//	notify derived classes that a new document has been parsed.
	return new_document_parsed();
}

bool GridReaderUGX::
new_document_parsed()
{
//	update entries
	m_entries.clear();

//	iterate through all grids
	xml_node<>* curNode = m_doc.first_node("grid");
	while(curNode){
		m_entries.push_back(GridEntry(curNode));
		GridEntry& gridEntry = m_entries.back();

	//	collect associated subset handlers
		xml_node<>* curSHNode = curNode->first_node("subset_handler");
		while(curSHNode){
			gridEntry.subsetHandlerEntries.push_back(SubsetHandlerEntry(curSHNode));
			curSHNode = curSHNode->next_sibling("subset_handler");
		}

	//	collect associated selectors
		xml_node<>* curSelNode = curNode->first_node("selector");
		while(curSelNode){
			gridEntry.selectorEntries.push_back(SelectorEntry(curSelNode));
			curSelNode = curSelNode->next_sibling("selector");
		}

	//	collect associated projectionHandlers
		xml_node<>* curPHNode = curNode->first_node("projection_handler");
		while(curPHNode){
			gridEntry.projectionHandlerEntries.push_back(curPHNode);
			curPHNode = curPHNode->next_sibling("projection_handler");
		}

		curNode = curNode->next_sibling("grid");
	}

	return true;
}

bool GridReaderUGX::
create_edges(std::vector<Edge*>& edgesOut,
			Grid& grid, rapidxml::xml_node<>* node,
			std::vector<Vertex*>& vrts)
{
//	create a buffer with which we can access the data
	string str(node->value(), node->value_size());
	stringstream ss(str, ios_base::in);

//	read the edges
	int i1, i2;
	while(!ss.eof()){
	//	read the indices
		ss >> i1 >> i2;

	//	make sure that everything went right
		if(ss.fail())
			break;

	//	make sure that the indices are valid
		int maxInd = (int)vrts.size() - 1;
		if(i1 < 0 || i1 > maxInd ||
		   i2 < 0 || i2 > maxInd)
		{
			UG_LOG("  ERROR in GridReaderUGX::create_edges: invalid vertex index: "
					"(" << i1 << ", " << i2 << ")\n");
			return false;
		}

	//	create the edge
		edgesOut.push_back(*grid.create<RegularEdge>(EdgeDescriptor(vrts[i1], vrts[i2])));
	}

	return true;
}

bool GridReaderUGX::
create_constraining_edges(std::vector<Edge*>& edgesOut,
						  Grid& grid, rapidxml::xml_node<>* node,
			 			  std::vector<Vertex*>& vrts)
{
//	create a buffer with which we can access the data
	string str(node->value(), node->value_size());
	stringstream ss(str, ios_base::in);

//	read the edges
	int i1, i2;
	while(!ss.eof()){
	//	read the indices
		ss >> i1 >> i2;

	//	make sure that everything went right
		if(ss.fail())
			break;

	//	make sure that the indices are valid
		int maxInd = (int)vrts.size() - 1;
		if(i1 < 0 || i1 > maxInd ||
		   i2 < 0 || i2 > maxInd)
		{
			UG_LOG("  ERROR in GridReaderUGX::create_constraining_edges: invalid vertex index.\n");
			return false;
		}

	//	create the edge
		edgesOut.push_back(*grid.create<ConstrainingEdge>(EdgeDescriptor(vrts[i1], vrts[i2])));
	}

	return true;
}

bool GridReaderUGX::
create_constrained_edges(std::vector<Edge*>& edgesOut,
						  std::vector<std::pair<int, int> >& constrainingObjsOut,
						  Grid& grid, rapidxml::xml_node<>* node,
			 			  std::vector<Vertex*>& vrts)
{
//	create a buffer with which we can access the data
	string str(node->value(), node->value_size());
	stringstream ss(str, ios_base::in);

//	read the edges
	int i1, i2;
	while(!ss.eof()){
	//	read the indices
		ss >> i1 >> i2;

	//	read the type and index of the constraining object
		int conObjType, conObjIndex;
		ss >> conObjType;

		if(conObjType != -1)
			ss >> conObjIndex;

	//	make sure that everything went right
		if(ss.fail())
			break;

	//	make sure that the indices are valid
		int maxInd = (int)vrts.size() - 1;
		if(i1 < 0 || i1 > maxInd ||
		   i2 < 0 || i2 > maxInd)
		{
			UG_LOG("  ERROR in GridReaderUGX::create_edges: invalid vertex index.\n");
			return false;
		}

	//	create the edge
		ConstrainedEdge* edge = *grid.create<ConstrainedEdge>(EdgeDescriptor(vrts[i1], vrts[i2]));
		edgesOut.push_back(edge);

	//	add conObjType and conObjIndex to their list
		constrainingObjsOut.push_back(std::make_pair(conObjType, conObjIndex));
	}

	return true;
}

bool GridReaderUGX::
create_triangles(std::vector<Face*>& facesOut,
				  Grid& grid, rapidxml::xml_node<>* node,
				  std::vector<Vertex*>& vrts)
{
//	create a buffer with which we can access the data
	string str(node->value(), node->value_size());
	stringstream ss(str, ios_base::in);

//	read the triangles
	int i1, i2, i3;
	while(!ss.eof()){
	//	read the indices
		ss >> i1 >> i2 >> i3;

	//	make sure that everything went right
		if(ss.fail())
			break;

	//	make sure that the indices are valid
		int maxInd = (int)vrts.size() - 1;
		if(i1 < 0 || i1 > maxInd ||
		   i2 < 0 || i2 > maxInd ||
		   i3 < 0 || i3 > maxInd)
		{
			UG_LOG("  ERROR in GridReaderUGX::create_triangles: invalid vertex index.\n");
			return false;
		}

	//	create the triangle
		facesOut.push_back(
			*grid.create<Triangle>(TriangleDescriptor(vrts[i1], vrts[i2], vrts[i3])));
	}

	return true;
}

bool GridReaderUGX::
create_constraining_triangles(std::vector<Face*>& facesOut,
					  Grid& grid, rapidxml::xml_node<>* node,
					  std::vector<Vertex*>& vrts)
{
//	create a buffer with which we can access the data
	string str(node->value(), node->value_size());
	stringstream ss(str, ios_base::in);

//	read the triangles
	int i1, i2, i3;
	while(!ss.eof()){
	//	read the indices
		ss >> i1 >> i2 >> i3;

	//	make sure that everything went right
		if(ss.fail())
			break;

	//	make sure that the indices are valid
		int maxInd = (int)vrts.size() - 1;
		if(i1 < 0 || i1 > maxInd ||
		   i2 < 0 || i2 > maxInd ||
		   i3 < 0 || i3 > maxInd)
		{
			UG_LOG("  ERROR in GridReaderUGX::create_constraining_triangles: invalid vertex index.\n");
			return false;
		}

	//	create the triangle
		facesOut.push_back(
			*grid.create<ConstrainingTriangle>(TriangleDescriptor(vrts[i1], vrts[i2], vrts[i3])));
	}

	return true;
}

bool GridReaderUGX::
create_constrained_triangles(std::vector<Face*>& facesOut,
					  std::vector<std::pair<int, int> >& constrainingObjsOut,
					  Grid& grid, rapidxml::xml_node<>* node,
					  std::vector<Vertex*>& vrts)
{
//	create a buffer with which we can access the data
	string str(node->value(), node->value_size());
	stringstream ss(str, ios_base::in);

//	read the triangles
	int i1, i2, i3;
	while(!ss.eof()){
	//	read the indices
		ss >> i1 >> i2 >> i3;

	//	read the type and index of the constraining object
		int conObjType, conObjIndex;
		ss >> conObjType;

		if(conObjType != -1)
			ss >> conObjIndex;
			
	//	make sure that everything went right
		if(ss.fail())
			break;

	//	make sure that the indices are valid
		int maxInd = (int)vrts.size() - 1;
		if(i1 < 0 || i1 > maxInd ||
		   i2 < 0 || i2 > maxInd ||
		   i3 < 0 || i3 > maxInd)
		{
			UG_LOG("  ERROR in GridReaderUGX::create_constraining_triangles: invalid vertex index.\n");
			return false;
		}

	//	create the triangle
		facesOut.push_back(
			*grid.create<ConstrainedTriangle>(TriangleDescriptor(vrts[i1], vrts[i2], vrts[i3])));
			
	//	add conObjType and conObjIndex to their list
		constrainingObjsOut.push_back(std::make_pair(conObjType, conObjIndex));
	}

	return true;
}

bool GridReaderUGX::
create_quadrilaterals(std::vector<Face*>& facesOut,
					   Grid& grid, rapidxml::xml_node<>* node,
					   std::vector<Vertex*>& vrts)
{
//	create a buffer with which we can access the data
	string str(node->value(), node->value_size());
	stringstream ss(str, ios_base::in);

//	read the quadrilaterals
	int i1, i2, i3, i4;
	while(!ss.eof()){
	//	read the indices
		ss >> i1 >> i2 >> i3 >> i4;

	//	make sure that everything went right
		if(ss.fail())
			break;

	//	make sure that the indices are valid
		int maxInd = (int)vrts.size() - 1;
		if(i1 < 0 || i1 > maxInd ||
		   i2 < 0 || i2 > maxInd ||
		   i3 < 0 || i3 > maxInd ||
		   i4 < 0 || i4 > maxInd)
		{
			UG_LOG("  ERROR in GridReaderUGX::create_quadrilaterals: invalid vertex index.\n");
			return false;
		}

	//	create the quad
		facesOut.push_back(
			*grid.create<Quadrilateral>(QuadrilateralDescriptor(vrts[i1], vrts[i2],
															   vrts[i3], vrts[i4])));
	}

	return true;
}

bool GridReaderUGX::
create_constraining_quadrilaterals(std::vector<Face*>& facesOut,
					  Grid& grid, rapidxml::xml_node<>* node,
					  std::vector<Vertex*>& vrts)
{
//	create a buffer with which we can access the data
	string str(node->value(), node->value_size());
	stringstream ss(str, ios_base::in);

//	read the quadrilaterals
	int i1, i2, i3, i4;
	while(!ss.eof()){
	//	read the indices
		ss >> i1 >> i2 >> i3 >> i4;

	//	make sure that everything went right
		if(ss.fail())
			break;

	//	make sure that the indices are valid
		int maxInd = (int)vrts.size() - 1;
		if(i1 < 0 || i1 > maxInd ||
		   i2 < 0 || i2 > maxInd ||
		   i3 < 0 || i3 > maxInd ||
		   i4 < 0 || i4 > maxInd)
		{
			UG_LOG("  ERROR in GridReaderUGX::create_quadrilaterals: invalid vertex index.\n");
			return false;
		}

	//	create the quad
		facesOut.push_back(
			*grid.create<ConstrainingQuadrilateral>(QuadrilateralDescriptor(
															vrts[i1], vrts[i2],
															vrts[i3], vrts[i4])));
	}

	return true;
}

bool GridReaderUGX::
create_constrained_quadrilaterals(std::vector<Face*>& facesOut,
					  std::vector<std::pair<int, int> >& constrainingObjsOut,
					  Grid& grid, rapidxml::xml_node<>* node,
					  std::vector<Vertex*>& vrts)
{
//	create a buffer with which we can access the data
	string str(node->value(), node->value_size());
	stringstream ss(str, ios_base::in);

//	read the quadrilaterals
	int i1, i2, i3, i4;
	while(!ss.eof()){
	//	read the indices
		ss >> i1 >> i2 >> i3 >> i4;

	//	read the type and index of the constraining object
		int conObjType, conObjIndex;
		ss >> conObjType;

		if(conObjType != -1)
			ss >> conObjIndex;
			
	//	make sure that everything went right
		if(ss.fail())
			break;

	//	make sure that the indices are valid
		int maxInd = (int)vrts.size() - 1;
		if(i1 < 0 || i1 > maxInd ||
		   i2 < 0 || i2 > maxInd ||
		   i3 < 0 || i3 > maxInd ||
		   i4 < 0 || i4 > maxInd)
		{
			UG_LOG("  ERROR in GridReaderUGX::create_quadrilaterals: invalid vertex index.\n");
			return false;
		}

	//	create the quad
		facesOut.push_back(
			*grid.create<ConstrainedQuadrilateral>(QuadrilateralDescriptor(
															vrts[i1], vrts[i2],
															vrts[i3], vrts[i4])));
	
	//	add conObjType and conObjIndex to their list
		constrainingObjsOut.push_back(std::make_pair(conObjType, conObjIndex));
	}

	return true;
}

					  
bool GridReaderUGX::
create_tetrahedrons(std::vector<Volume*>& volsOut,
					 Grid& grid, rapidxml::xml_node<>* node,
					 std::vector<Vertex*>& vrts)
{
//	create a buffer with which we can access the data
	string str(node->value(), node->value_size());
	stringstream ss(str, ios_base::in);

//	read the tetrahedrons
	int i1, i2, i3, i4;
	while(!ss.eof()){
	//	read the indices
		ss >> i1 >> i2 >> i3 >> i4;

	//	make sure that everything went right
		if(ss.fail())
			break;

	//	make sure that the indices are valid
		int maxInd = (int)vrts.size() - 1;
		if(i1 < 0 || i1 > maxInd ||
		   i2 < 0 || i2 > maxInd ||
		   i3 < 0 || i3 > maxInd ||
		   i4 < 0 || i4 > maxInd)
		{
			UG_LOG("  ERROR in GridReaderUGX::create_tetrahedrons: invalid vertex index.\n");
			return false;
		}

	//	create the element
		volsOut.push_back(
			*grid.create<Tetrahedron>(TetrahedronDescriptor(vrts[i1], vrts[i2],
														   vrts[i3], vrts[i4])));
	}

	return true;
}

bool GridReaderUGX::
create_hexahedrons(std::vector<Volume*>& volsOut,
					Grid& grid, rapidxml::xml_node<>* node,
					std::vector<Vertex*>& vrts)
{
//	create a buffer with which we can access the data
	string str(node->value(), node->value_size());
	stringstream ss(str, ios_base::in);

//	read the hexahedrons
	int i1, i2, i3, i4, i5, i6, i7, i8;
	while(!ss.eof()){
	//	read the indices
		ss >> i1 >> i2 >> i3 >> i4 >> i5 >> i6 >> i7 >> i8;

	//	make sure that everything went right
		if(ss.fail())
			break;

	//	make sure that the indices are valid
		int maxInd = (int)vrts.size() - 1;
		if(i1 < 0 || i1 > maxInd ||
		   i2 < 0 || i2 > maxInd ||
		   i3 < 0 || i3 > maxInd ||
		   i4 < 0 || i4 > maxInd ||
		   i5 < 0 || i5 > maxInd ||
		   i6 < 0 || i6 > maxInd ||
		   i7 < 0 || i7 > maxInd ||
		   i8 < 0 || i8 > maxInd)
		{
			UG_LOG("  ERROR in GridReaderUGX::create_hexahedrons: invalid vertex index.\n");
			return false;
		}

	//	create the element
		volsOut.push_back(
			*grid.create<Hexahedron>(HexahedronDescriptor(vrts[i1], vrts[i2], vrts[i3], vrts[i4],
														  vrts[i5], vrts[i6], vrts[i7], vrts[i8])));
	}

	return true;
}

bool GridReaderUGX::
create_prisms(std::vector<Volume*>& volsOut,
			  Grid& grid, rapidxml::xml_node<>* node,
			  std::vector<Vertex*>& vrts)
{
//	create a buffer with which we can access the data
	string str(node->value(), node->value_size());
	stringstream ss(str, ios_base::in);

//	read the hexahedrons
	int i1, i2, i3, i4, i5, i6;
	while(!ss.eof()){
	//	read the indices
		ss >> i1 >> i2 >> i3 >> i4 >> i5 >> i6;

	//	make sure that everything went right
		if(ss.fail())
			break;

	//	make sure that the indices are valid
		int maxInd = (int)vrts.size() - 1;
		if(i1 < 0 || i1 > maxInd ||
		   i2 < 0 || i2 > maxInd ||
		   i3 < 0 || i3 > maxInd ||
		   i4 < 0 || i4 > maxInd ||
		   i5 < 0 || i5 > maxInd ||
		   i6 < 0 || i6 > maxInd)
		{
			UG_LOG("  ERROR in GridReaderUGX::create_prisms: invalid vertex index.\n");
			return false;
		}

	//	create the element
		volsOut.push_back(
			*grid.create<Prism>(PrismDescriptor(vrts[i1], vrts[i2], vrts[i3], vrts[i4],
												vrts[i5], vrts[i6])));
	}

	return true;
}

bool GridReaderUGX::
create_pyramids(std::vector<Volume*>& volsOut,
				Grid& grid, rapidxml::xml_node<>* node,
				std::vector<Vertex*>& vrts)
{
//	create a buffer with which we can access the data
	string str(node->value(), node->value_size());
	stringstream ss(str, ios_base::in);

//	read the hexahedrons
	int i1, i2, i3, i4, i5;
	while(!ss.eof()){
	//	read the indices
		ss >> i1 >> i2 >> i3 >> i4 >> i5;

	//	make sure that everything went right
		if(ss.fail())
			break;

	//	make sure that the indices are valid
		int maxInd = (int)vrts.size() - 1;
		if(i1 < 0 || i1 > maxInd ||
		   i2 < 0 || i2 > maxInd ||
		   i3 < 0 || i3 > maxInd ||
		   i4 < 0 || i4 > maxInd ||
		   i5 < 0 || i5 > maxInd)
		{
			UG_LOG("  ERROR in GridReaderUGX::create_pyramids: invalid vertex index.\n");
			return false;
		}

	//	create the element
		volsOut.push_back(
			*grid.create<Pyramid>(PyramidDescriptor(vrts[i1], vrts[i2], vrts[i3],
													vrts[i4], vrts[i5])));
	}

	return true;
}

bool GridReaderUGX::
create_octahedrons(std::vector<Volume*>& volsOut,
					Grid& grid, rapidxml::xml_node<>* node,
					std::vector<Vertex*>& vrts)
{
//	create a buffer with which we can access the data
	string str(node->value(), node->value_size());
	stringstream ss(str, ios_base::in);

//	read the octahedrons
	int i1, i2, i3, i4, i5, i6;
	while(!ss.eof()){
	//	read the indices
		ss >> i1 >> i2 >> i3 >> i4 >> i5 >> i6;

	//	make sure that everything went right
		if(ss.fail())
			break;

	//	make sure that the indices are valid
		int maxInd = (int)vrts.size() - 1;
		if(i1 < 0 || i1 > maxInd ||
		   i2 < 0 || i2 > maxInd ||
		   i3 < 0 || i3 > maxInd ||
		   i4 < 0 || i4 > maxInd ||
		   i5 < 0 || i5 > maxInd ||
		   i6 < 0 || i6 > maxInd)
		{
			UG_LOG("  ERROR in GridReaderUGX::create_octahedrons: invalid vertex index.\n");
			return false;
		}

	//	create the element
		volsOut.push_back(
			*grid.create<Octahedron>(OctahedronDescriptor(	vrts[i1], vrts[i2], vrts[i3],
															vrts[i4], vrts[i5], vrts[i6])));
	}

	return true;
}


UGXFileInfo::UGXFileInfo() :
	m_fileParsed(false)
{
}

bool UGXFileInfo::parse_file(const char* filename)
{
	PROFILE_FUNC_GROUP("UGXFileInfo");
	string tfile = FindFileInStandardPaths(filename);

	ifstream in(tfile.c_str(), ios::binary);
	UG_COND_THROW(!in, "UGXFileInfo: couldn't find file '" << filename << "'");

//	get the length of the file
	streampos posStart = in.tellg();
	in.seekg(0, ios_base::end);
	streampos posEnd = in.tellg();
	streamsize size = posEnd - posStart;

//	go back to the start of the file
	in.seekg(posStart);

//	read the whole file en-block and terminate it with 0
	rapidxml::xml_document<> doc;
	char* fileContent = doc.allocate_string(0, size + 1);
	in.read(fileContent, size);
	fileContent[size] = 0;
	in.close();

//	parse the xml-data
	doc.parse<0>(fileContent);

	xml_node<>* curNode = doc.first_node("grid");
	while(curNode){
		m_grids.push_back(GridInfo());
		GridInfo& gInfo = m_grids.back();
		gInfo.m_name = node_name(curNode);

	//	collect associated subset handlers
		xml_node<>* curSHNode = curNode->first_node("subset_handler");
		while(curSHNode){
			gInfo.m_subsetHandlers.push_back(SubsetHandlerInfo());
			SubsetHandlerInfo& shInfo = gInfo.m_subsetHandlers.back();
			shInfo.m_name = node_name(curSHNode);

			xml_node<>* curSubsetNode = curSHNode->first_node("subset");

			while(curSubsetNode){
				shInfo.m_subsets.push_back(SubsetInfo());
				SubsetInfo& sInfo = shInfo.m_subsets.back();
				sInfo.m_name = node_name(curSubsetNode);
				curSubsetNode = curSubsetNode->next_sibling("subset");
			}

			curSHNode = curSHNode->next_sibling("subset_handler");
		}

		// loop through vertices to find bounding box of geometry
		AABox<vector3> box(vector3(0, 0, 0), vector3(0, 0, 0));
		xml_node<>* vrtNode = curNode->first_node("vertices");
		while (vrtNode)
		{
			// create a bounding box around the vertices contained in this xml node
			AABox<vector3> newBox;
			bool validBox = calculate_vertex_node_bbox(vrtNode, newBox);
		    if (validBox)
		    	box = AABox<vector3>(box, newBox);

		    vrtNode = vrtNode->next_sibling("vertices");
		}

		// TODO: Do we have to consider ConstrainedVertices here?

		gInfo.m_extension = box.extension();

		//	fill m_hasVertices, ...
		gInfo.m_hasVertices = curNode->first_node("vertices") != NULL;
		gInfo.m_hasVertices |= curNode->first_node("constrained_vertices") != NULL;

		gInfo.m_hasEdges = curNode->first_node("edges") != NULL;
		gInfo.m_hasEdges |= curNode->first_node("constraining_edges") != NULL;
		gInfo.m_hasEdges |= curNode->first_node("constrained_edges") != NULL;

		gInfo.m_hasFaces = curNode->first_node("triangles") != NULL;
		gInfo.m_hasFaces |= curNode->first_node("constraining_triangles") != NULL;
		gInfo.m_hasFaces |= curNode->first_node("constrained_triangles") != NULL;
		gInfo.m_hasFaces |= curNode->first_node("quadrilaterals") != NULL;
		gInfo.m_hasFaces |= curNode->first_node("constraining_quadrilaterals") != NULL;
		gInfo.m_hasFaces |= curNode->first_node("constrained_quadrilaterals") != NULL;

		gInfo.m_hasVolumes = curNode->first_node("tetrahedrons") != NULL;
		gInfo.m_hasVolumes |= curNode->first_node("hexahedrons") != NULL;
		gInfo.m_hasVolumes |= curNode->first_node("prisms") != NULL;
		gInfo.m_hasVolumes |= curNode->first_node("pyramids") != NULL;

		curNode = curNode->next_sibling("grid");
	}

	m_fileParsed = true;
	return true;
}

size_t UGXFileInfo::num_grids() const
{
	check_file_parsed();
	return m_grids.size();
}

size_t UGXFileInfo::num_subset_handlers(size_t gridInd) const
{
	return grid_info(gridInd).m_subsetHandlers.size();
}

size_t UGXFileInfo::num_subsets(size_t gridInd, size_t shInd) const
{
	return subset_handler_info(gridInd, shInd).m_subsets.size();
}

std::string UGXFileInfo::grid_name(size_t gridInd) const
{
	return grid_info(gridInd).m_name;
}

std::string UGXFileInfo::subset_handler_name(size_t gridInd, size_t shInd) const
{
	return subset_handler_info(gridInd, shInd).m_name;
}

std::string UGXFileInfo::subset_name(size_t gridInd, size_t shInd, size_t subsetInd) const
{
	return subset_info(gridInd, shInd, subsetInd).m_name;
}

bool UGXFileInfo::grid_has_vertices(size_t gridInd) const
{
	return grid_info(gridInd).m_hasVertices;
}

bool UGXFileInfo::grid_has_edges(size_t gridInd) const
{
	return grid_info(gridInd).m_hasEdges;
}

bool UGXFileInfo::grid_has_faces(size_t gridInd) const
{
	return grid_info(gridInd).m_hasFaces;
}

bool UGXFileInfo::grid_has_volumes(size_t gridInd) const
{
	return grid_info(gridInd).m_hasVolumes;
}

size_t UGXFileInfo::physical_grid_dimension(size_t gridInd) const
{
	const GridInfo& gi = grid_info(gridInd);

	const vector3& ext = gi.m_extension;
	const number relSmall = SMALL * std::max(ext[0], std::max(ext[1], ext[2]));
	for (int i = 2; i >= 0; --i)
	{
		if (ext[i] > relSmall)
			return (size_t) (i + 1);
	}
	return 0;
}

size_t UGXFileInfo::topological_grid_dimension(size_t gridInd) const
{
	const GridInfo& gi = grid_info(gridInd);

	if(gi.m_hasVolumes)
		return 3;
	if(gi.m_hasFaces)
		return 2;
	if(gi.m_hasEdges)
		return 1;

	return 0;
}

size_t UGXFileInfo::grid_world_dimension(size_t gridInd) const
{
	return physical_grid_dimension(gridInd);
}

std::string UGXFileInfo::node_name(rapidxml::xml_node<>* n) const
{
	xml_attribute<>* attrib = n->first_attribute("name");
	if(attrib)
		return attrib->value();
	return "";
}

void UGXFileInfo::check_file_parsed() const
{
	if(!m_fileParsed){
		UG_THROW("UGXFileInfo: no file has been parsed!");
	}
}

const UGXFileInfo::GridInfo&
UGXFileInfo::grid_info(size_t index) const
{
	check_file_parsed();
	if(index >= m_grids.size()){
		UG_THROW("Grid index out of range: " << index
				 << ". Num grids available: " << m_grids.size());
	}

	return m_grids[index];
}

const UGXFileInfo::SubsetHandlerInfo&
UGXFileInfo::subset_handler_info(size_t gridInd, size_t shInd) const
{
	const GridInfo& gi = grid_info(gridInd);
	if(shInd >= gi.m_subsetHandlers.size()){
		UG_THROW("SubsetHandler index out of range: " << shInd
				 << ". Num subset-handlers available: "
				 << gi.m_subsetHandlers.size());
	}

	return gi.m_subsetHandlers[shInd];
}

const UGXFileInfo::SubsetInfo&
UGXFileInfo::subset_info(size_t gridInd, size_t shInd, size_t subsetInd) const
{
	const SubsetHandlerInfo& shInfo = subset_handler_info(gridInd, shInd);
	if(subsetInd >= shInfo.m_subsets.size()){
		UG_THROW("Subset index out of range: " << subsetInd
				 << ". Num subset available: "
				 << shInfo.m_subsets.size());
	}

	return shInfo.m_subsets[subsetInd];
}

bool
UGXFileInfo::calculate_vertex_node_bbox(rapidxml::xml_node<>* vrtNode, AABox<vector3>& bb) const
{
	size_t numSrcCoords = 0;
	rapidxml::xml_attribute<>* attrib = vrtNode->first_attribute("coords");
	if (!attrib) return false;

	numSrcCoords = std::strtoul(attrib->value(), NULL, 10);
	UG_ASSERT(errno != ERANGE, "Coordinate dimension in .ugx file is out of range.");
	UG_ASSERT(numSrcCoords <= 3,
			  "Coordinate dimension in .ugx file needs to be in {0,1,2,3}, but is "
			  << numSrcCoords << ".");

	if (numSrcCoords > 3)
		return false;

//	create a buffer with which we can access the data
	std::string str(vrtNode->value(), vrtNode->value_size());
	std::stringstream ss(str, std::ios_base::in);

	AABox<vector3> box(vector3(0, 0, 0), vector3(0, 0, 0));
	vector3 min(0, 0, 0);
	vector3 max(0, 0, 0);
	vector3 vrt(0, 0, 0);
	size_t nVrt = 0;
	while (!ss.eof())
	{
		for (size_t i = 0; i < numSrcCoords; ++i)
			ss >> vrt[i];

		if (ss.fail())
			break;

		++nVrt;

		for (size_t j = 0; j < numSrcCoords; ++j)
		{
			min[j] = std::min(min[j], vrt[j]);
			max[j] = std::max(max[j], vrt[j]);
		}
	}

	// create bounding box
	bb = AABox<vector3>(min, max);

	return nVrt > 0;
}






////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//	implementation of GridReaderUGX
template <class TPositionAttachment>
bool GridReaderUGX::
grid(Grid& gridOut, size_t index,
		  TPositionAttachment& aPos)
{
	using namespace rapidxml;
	using namespace std;

//	make sure that a node at the given index exists
	if(num_grids() <= index){
		UG_LOG("  GridReaderUGX::read: bad grid index!\n");
		return false;
	}

	Grid& grid = gridOut;

//	Since we have to create all elements in the correct order and
//	since we have to make sure that no elements are created in between,
//	we'll first disable all grid-options and reenable them later on
	uint gridopts = grid.get_options();
	grid.set_options(GRIDOPT_NONE);

//	access node data
	if(!grid.has_vertex_attachment(aPos)){
		grid.attach_to_vertices(aPos);
	}

	Grid::VertexAttachmentAccessor<TPositionAttachment> aaPos(grid, aPos);

//	store the grid in the grid-vector and assign indices to the vertices
	m_entries[index].grid = &grid;

//	get the grid-node and the vertex-vector
	xml_node<>* gridNode = m_entries[index].node;
	vector<Vertex*>& vertices = m_entries[index].vertices;
	vector<Edge*>& edges = m_entries[index].edges;
	vector<Face*>& faces = m_entries[index].faces;
	vector<Volume*>& volumes = m_entries[index].volumes;

//	we'll record constraining objects for constrained-vertices and constrained-edges
	std::vector<std::pair<int, int> > constrainingObjsVRT;
	std::vector<std::pair<int, int> > constrainingObjsEDGE;
	std::vector<std::pair<int, int> > constrainingObjsTRI;
	std::vector<std::pair<int, int> > constrainingObjsQUAD;
	
//	iterate through the nodes in the grid and create the entries
	xml_node<>* curNode = gridNode->first_node();
	for(;curNode; curNode = curNode->next_sibling()){
		bool bSuccess = true;
		const char* name = curNode->name();
		if(strcmp(name, "vertices") == 0)
			bSuccess = create_vertices(vertices, grid, curNode, aaPos);
		else if(strcmp(name, "constrained_vertices") == 0)
			bSuccess = create_constrained_vertices(vertices, constrainingObjsVRT,
												   grid, curNode, aaPos);
		else if(strcmp(name, "edges") == 0)
			bSuccess = create_edges(edges, grid, curNode, vertices);
		else if(strcmp(name, "constraining_edges") == 0)
			bSuccess = create_constraining_edges(edges, grid, curNode, vertices);
		else if(strcmp(name, "constrained_edges") == 0)
			bSuccess = create_constrained_edges(edges, constrainingObjsEDGE,
												grid, curNode, vertices);
		else if(strcmp(name, "triangles") == 0)
			bSuccess = create_triangles(faces, grid, curNode, vertices);
		else if(strcmp(name, "constraining_triangles") == 0)
			bSuccess = create_constraining_triangles(faces, grid, curNode, vertices);
		else if(strcmp(name, "constrained_triangles") == 0)
			bSuccess = create_constrained_triangles(faces, constrainingObjsTRI,
												grid, curNode, vertices);
		else if(strcmp(name, "quadrilaterals") == 0)
			bSuccess = create_quadrilaterals(faces, grid, curNode, vertices);
		else if(strcmp(name, "constraining_quadrilaterals") == 0)
			bSuccess = create_constraining_quadrilaterals(faces, grid, curNode, vertices);
		else if(strcmp(name, "constrained_quadrilaterals") == 0)
			bSuccess = create_constrained_quadrilaterals(faces, constrainingObjsQUAD,
												grid, curNode, vertices);
		else if(strcmp(name, "tetrahedrons") == 0)
			bSuccess = create_tetrahedrons(volumes, grid, curNode, vertices);
		else if(strcmp(name, "hexahedrons") == 0)
			bSuccess = create_hexahedrons(volumes, grid, curNode, vertices);
		else if(strcmp(name, "prisms") == 0)
			bSuccess = create_prisms(volumes, grid, curNode, vertices);
		else if(strcmp(name, "pyramids") == 0)
			bSuccess = create_pyramids(volumes, grid, curNode, vertices);
		else if(strcmp(name, "octahedrons") == 0)
			bSuccess = create_octahedrons(volumes, grid, curNode, vertices);

		else if(strcmp(name, "vertex_attachment") == 0)
			bSuccess = read_attachment<Vertex>(grid, curNode);
		else if(strcmp(name, "edge_attachment") == 0)
			bSuccess = read_attachment<Edge>(grid, curNode);
		else if(strcmp(name, "face_attachment") == 0)
			bSuccess = read_attachment<Face>(grid, curNode);
		else if(strcmp(name, "volume_attachment") == 0)
			bSuccess = read_attachment<Volume>(grid, curNode);


		if(!bSuccess){
			grid.set_options(gridopts);
			return false;
		}
	}
	
//	resolve constrained object relations
	if(!constrainingObjsVRT.empty()){
		//UG_LOG("num-edges: " << edges.size() << std::endl);
	//	iterate over the pairs.
	//	at the same time we'll iterate over the constrained vertices since
	//	they are synchronized.
		ConstrainedVertexIterator hvIter = grid.begin<ConstrainedVertex>();
		for(std::vector<std::pair<int, int> >::iterator iter = constrainingObjsVRT.begin();
			iter != constrainingObjsVRT.end(); ++iter, ++hvIter)
		{
			ConstrainedVertex* hv = *hvIter;
			
			switch(iter->first){
				case 1:	// constraining object is an edge
				{
				//	make sure that the index is valid
					if(iter->second >= 0 && iter->second < (int)edges.size()){
					//	get the edge
						ConstrainingEdge* edge = dynamic_cast<ConstrainingEdge*>(edges[iter->second]);
						if(edge){
							hv->set_constraining_object(edge);
							edge->add_constrained_object(hv);
						}
						else{
							UG_LOG("WARNING: Type-ID / type mismatch. Ignoring edge " << iter->second << ".\n");
						}
					}
					else{
						UG_LOG("ERROR in GridReaderUGX: Bad edge index in constrained vertex: " << iter->second << "\n");
					}
				}break;
				
				case 2:	// constraining object is an face
				{
				//	make sure that the index is valid
					if(iter->second >= 0 && iter->second < (int)faces.size()){
					//	get the edge
						ConstrainingFace* face = dynamic_cast<ConstrainingFace*>(faces[iter->second]);
						if(face){
							hv->set_constraining_object(face);
							face->add_constrained_object(hv);
						}
						else{
							UG_LOG("WARNING in GridReaderUGX: Type-ID / type mismatch. Ignoring face " << iter->second << ".\n");
						}
					}
					else{
						UG_LOG("ERROR in GridReaderUGX: Bad face index in constrained vertex: " << iter->second << "\n");
					}
				}break;
				
				default:
				{
//					UG_LOG("WARNING in GridReaderUGX: unsupported type-id of constraining vertex"
//							<< " at " << GetGridObjectCenter(grid, hv) << "\n");
					break;
				}
			}
		}
	}

	if(!constrainingObjsEDGE.empty()){
	//	iterate over the pairs.
	//	at the same time we'll iterate over the constrained vertices since
	//	they are synchronized.
		ConstrainedEdgeIterator ceIter = grid.begin<ConstrainedEdge>();
		for(std::vector<std::pair<int, int> >::iterator iter = constrainingObjsEDGE.begin();
			iter != constrainingObjsEDGE.end(); ++iter, ++ceIter)
		{
			ConstrainedEdge* ce = *ceIter;
			
			switch(iter->first){
				case 1:	// constraining object is an edge
				{
				//	make sure that the index is valid
					if(iter->second >= 0 && iter->second < (int)edges.size()){
					//	get the edge
						ConstrainingEdge* edge = dynamic_cast<ConstrainingEdge*>(edges[iter->second]);
						if(edge){
							ce->set_constraining_object(edge);
							edge->add_constrained_object(ce);
						}
						else{
							UG_LOG("WARNING in GridReaderUGX: Type-ID / type mismatch. Ignoring edge " << iter->second << ".\n");
						}
					}
					else{
						UG_LOG("ERROR in GridReaderUGX: Bad edge index in constrained edge.\n");
					}
				}break;
				case 2:	// constraining object is an face
				{
				//	make sure that the index is valid
					if(iter->second >= 0 && iter->second < (int)faces.size()){
					//	get the edge
						ConstrainingFace* face = dynamic_cast<ConstrainingFace*>(faces[iter->second]);
						if(face){
							ce->set_constraining_object(face);
							face->add_constrained_object(ce);
						}
						else{
							UG_LOG("WARNING in GridReaderUGX: Type-ID / type mismatch. Ignoring face " << iter->second << ".\n");
						}
					}
					else{
						UG_LOG("ERROR in GridReaderUGX: Bad face index in constrained edge: " << iter->second << "\n");
					}
				}break;
				
				default:
				{
//					UG_LOG("WARNING in GridReaderUGX: unsupported type-id of constraining edge"
//							<< " at " << GetGridObjectCenter(grid, ce) << "\n");
					break;
				}
			}
		}
	}

	if(!constrainingObjsTRI.empty()){
	//	iterate over the pairs.
	//	at the same time we'll iterate over the constrained vertices since
	//	they are synchronized.
		ConstrainedTriangleIterator cfIter = grid.begin<ConstrainedTriangle>();
		for(std::vector<std::pair<int, int> >::iterator iter = constrainingObjsTRI.begin();
			iter != constrainingObjsTRI.end(); ++iter, ++cfIter)
		{
			ConstrainedFace* cdf = *cfIter;
			
			switch(iter->first){
				case 2:	// constraining object is an face
				{
				//	make sure that the index is valid
					if(iter->second >= 0 && iter->second < (int)faces.size()){
					//	get the edge
						ConstrainingFace* face = dynamic_cast<ConstrainingFace*>(faces[iter->second]);
						if(face){
							cdf->set_constraining_object(face);
							face->add_constrained_object(cdf);
						}
						else{
							UG_LOG("WARNING in GridReaderUGX: Type-ID / type mismatch. Ignoring face " << iter->second << ".\n");
						}
					}
					else{
						UG_LOG("ERROR in GridReaderUGX: Bad face index in constrained face: " << iter->second << "\n");
					}
				}break;
				
				default:
				{
//					UG_LOG("WARNING in GridReaderUGX: unsupported type-id of constraining triangle"
//							<< " at " << GetGridObjectCenter(grid, cdf) << "\n");
					break;
				}
			}
		}
	}

	if(!constrainingObjsQUAD.empty()){
	//	iterate over the pairs.
	//	at the same time we'll iterate over the constrained vertices since
	//	they are synchronized.
		ConstrainedQuadrilateralIterator cfIter = grid.begin<ConstrainedQuadrilateral>();
		for(std::vector<std::pair<int, int> >::iterator iter = constrainingObjsQUAD.begin();
			iter != constrainingObjsQUAD.end(); ++iter, ++cfIter)
		{
			ConstrainedFace* cdf = *cfIter;
			
			switch(iter->first){
				case 2:	// constraining object is an face
				{
				//	make sure that the index is valid
					if(iter->second >= 0 && iter->second < (int)faces.size()){
					//	get the edge
						ConstrainingFace* face = dynamic_cast<ConstrainingFace*>(faces[iter->second]);
						if(face){
							cdf->set_constraining_object(face);
							face->add_constrained_object(cdf);
						}
						else{
							UG_LOG("WARNING in GridReaderUGX: Type-ID / type mismatch. Ignoring face " << iter->second << ".\n");
						}
					}
					else{
						UG_LOG("ERROR in GridReaderUGX: Bad face index in constrained face: " << iter->second << "\n");
					}
				}break;
				
				default:
				{
//					UG_LOG("WARNING in GridReaderUGX: unsupported type-id of constraining quadrilateral"
//							<< " at " << GetGridObjectCenter(grid, cdf) << "\n");
					break;
				}
			}
		}
	}

//	reenable the grids options.
	grid.set_options(gridopts);

	return true;
}

template <class TAAPos>
bool GridReaderUGX::
create_vertices(std::vector<Vertex*>& vrtsOut, Grid& grid,
				rapidxml::xml_node<>* vrtNode, TAAPos aaPos)
{
	using namespace rapidxml;
	using namespace std;

	int numSrcCoords = -1;
	xml_attribute<>* attrib = vrtNode->first_attribute("coords");
	if(attrib)
		numSrcCoords = atoi(attrib->value());

	int numDestCoords = (int)TAAPos::ValueType::Size;

	assert(numDestCoords > 0 && "bad position attachment type");

	if(numSrcCoords < 1 || numDestCoords < 1)
		return false;

//	create a buffer with which we can access the data
	string str(vrtNode->value(), vrtNode->value_size());
	stringstream ss(str, ios_base::in);

//	if numDestCoords == numSrcCoords parsing will be faster
	if(numSrcCoords == numDestCoords){
		while(!ss.eof()){
		//	read the data
			typename TAAPos::ValueType v;

			for(int i = 0; i < numSrcCoords; ++i)
				ss >> v[i];

		//	make sure that everything went right
			if(ss.fail())
				break;

		//	create a new vertex
			RegularVertex* vrt = *grid.create<RegularVertex>();
			vrtsOut.push_back(vrt);

		//	set the coordinates
			aaPos[vrt] = v;
		}
	}
	else{
	//	we have to be careful with reading.
	//	if numDestCoords < numSrcCoords we'll ignore some coords,
	//	in the other case we'll add some 0's.
		int minNumCoords = min(numSrcCoords, numDestCoords);
		typename TAAPos::ValueType::value_type dummy = 0;

		while(!ss.eof()){
		//	read the data
			typename TAAPos::ValueType v;

			int iMin;
			for(iMin = 0; iMin < minNumCoords; ++iMin)
				ss >> v[iMin];

		//	ignore unused entries in the input buffer
			for(int i = iMin; i < numSrcCoords; ++i)
				ss >> dummy;

		//	add 0's to the vector
			for(int i = iMin; i < numDestCoords; ++i)
				v[i] = 0;

		//	make sure that everything went right
			if(ss.fail())
				break;

		//	create a new vertex
			RegularVertex* vrt = *grid.create<RegularVertex>();
			vrtsOut.push_back(vrt);

		//	set the coordinates
			aaPos[vrt] = v;
		}
	}

	return true;
}

template <class TAAPos>
bool GridReaderUGX::
create_constrained_vertices(std::vector<Vertex*>& vrtsOut,
							std::vector<std::pair<int, int> >& constrainingObjsOut,
							Grid& grid, rapidxml::xml_node<>* vrtNode, TAAPos aaPos)
{
	using namespace rapidxml;
	using namespace std;

	int numSrcCoords = -1;
	xml_attribute<>* attrib = vrtNode->first_attribute("coords");
	if(attrib)
		numSrcCoords = atoi(attrib->value());

	int numDestCoords = (int)TAAPos::ValueType::Size;

	assert(numDestCoords > 0 && "bad position attachment type");

	if(numSrcCoords < 1 || numDestCoords < 1)
		return false;

//	create a buffer with which we can access the data
	string str(vrtNode->value(), vrtNode->value_size());
	stringstream ss(str, ios_base::in);

//	we have to be careful with reading.
//	if numDestCoords < numSrcCoords we'll ignore some coords,
//	in the other case we'll add some 0's.
	int minNumCoords = min(numSrcCoords, numDestCoords);
	typename TAAPos::ValueType::value_type dummy = 0;

//todo: speed could be improved, if dest and src position types have the same dimension
	while(!ss.eof()){
	//	read the data
		typename TAAPos::ValueType v;

		int iMin;
		for(iMin = 0; iMin < minNumCoords; ++iMin)
			ss >> v[iMin];

	//	ignore unused entries in the input buffer
		for(int i = iMin; i < numSrcCoords; ++i)
			ss >> dummy;

	//	add 0's to the vector
		for(int i = iMin; i < numDestCoords; ++i)
			v[i] = 0;

	//	now read the type of the constraining object and its index
		int conObjType = -1;
		int conObjIndex = -1;
		ss >> conObjType;
		
		if(conObjType != -1)
			ss >> conObjIndex;

	//	depending on the constrainings object type, we'll read local coordinates
		vector3 localCoords(0, 0, 0);
		switch(conObjType){
			case 1:	// an edge. read one local coord
				ss >> localCoords.x();
				break;
			case 2: // a face. read two local coords
				ss >> localCoords.x() >> localCoords.y();
				break;
			default:
				break;
		}
				
	//	make sure that everything went right
		if(ss.fail())
			break;

	//	create a new vertex
		ConstrainedVertex* vrt = *grid.create<ConstrainedVertex>();
		vrtsOut.push_back(vrt);

	//	set the coordinates
		aaPos[vrt] = v;
		
	//	set local coordinates
		vrt->set_local_coordinates(localCoords.x(), localCoords.y());
		
	//	add the constraining object id and index to the list
		constrainingObjsOut.push_back(std::make_pair(conObjType, conObjIndex));
	}

	return true;
}


template <class TElem>
bool GridReaderUGX::
read_attachment(Grid& grid, rapidxml::xml_node<>* node)
{
	using namespace rapidxml;
	using namespace std;

	xml_attribute<>* attribName = node->first_attribute("name");
	UG_COND_THROW(!attribName, "Invalid attachment entry: No 'name' attribute was supplied!");
	string name = attribName->value();

	xml_attribute<>* attribType = node->first_attribute("type");
	UG_COND_THROW(!attribType, "Invalid attachment entry: No 'type' attribute was supplied!");
	string type = attribType->value();

	bool global = false;
	if (xml_attribute<>* attrib = node->first_attribute("global")){
		global = bool(atoi(attrib->value()));
	}

	bool passOn = false;
	if (xml_attribute<>* attrib = node->first_attribute("passOn")){
		passOn = bool(atoi(attrib->value()));
	}
	
	if(global && !GlobalAttachments::is_declared(name)){
		if(GlobalAttachments::type_is_registered(type)){
			GlobalAttachments::declare_attachment(name, type, passOn);
			// GlobalAttachments::mark_attachment_as_locally_declared(name);
		}
		else
			return true;
	}

	UG_COND_THROW(type.compare(GlobalAttachments::type_name(name)) != 0,
				  "Attachment type mismatch. Expecting type: " << 
				  GlobalAttachments::type_name(name)
				  << ", but given type is: " << type);

	string str(node->value(), node->value_size());
	stringstream ss(str, ios_base::in);
	GlobalAttachments::read_attachment_values<TElem>(ss, grid, name);

	return true;
}
