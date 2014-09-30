/*--------------------------------------------------------------------------
History
--------

10/10/2013 - eliasb             - First version         
10/15/2013 - eliasb             - Added nodeloc and all_nodes lookups and corresponding lookup functions
10/21/2013 - eliasb             - Added groupnet_t class
                                - Change some type names
                                - Moved history comment block to the cpp file
10/22/2013 - eliasb             - Added gm.get_nodes()
                                - Change group def list type name
10/25/2013 - eliasb             - typedef nodedef_list_t * as pnodedef_list_t
10/28/2013 - eliasb             - made nodedef_t as a pointer in the nodedef_list class
10/29/2013 - eliasb             - skip trailing spaces while parsing
                                - added get_first_nd()
                                - got rid of the 'all_ndls' structure and added nid2nd structure
10/30/2013 - eliasb             - renamed classes to less confusing names: supergroup, groups, grouplist, nodedef
10/31/2013 - eliasb             - added get_first_node() helper function to nodegroup_t/nodegroup_list_t
                                - renamed png2nid_t to ng2nid_t
                                - added nodegroup_list_t::get_first_ng()								
11/01/2013 - eliasb             - groupman parser now supports two sections: similar nodes and node path information
11/04/2013 - eliasb             - added remove_supergroup()/combine_ngl() to groupman_t								
                                - added ngl::find_biggest() to find the bigest
                                - combine_ngl() now groups under the biggest NGL
                                - skip parsing lines on unknown sections
                                - fixed bug in emit file function: neither nodeset key nor group name were emitted
11/05/2013 - eliasb             - use std::list<> instead of qlist<> because list.remove() exists already (and also STL could be better)
11/06/2013 - eliasb             - added 'remove_sg', 'move_nodes_to_ng'
                                - added 'reset_groupping'
                                - added added nodegroup_list_t.add_nodegroup()
--------------------------------------------------------------------------*/

#define USE_STANDARD_FILE_FUNCTIONS
#define USE_DANGEROUS_FUNCTIONS
#include "groupman.h"
#include <fpro.h>
#include <string>
#include <fstream>
#include <iostream>
#include "util.h"

//--------------------------------------------------------------------------
static const char STR_ID[]          = "ID";
static const char STR_MATCH_COUNT[] = "MC";
static const char STR_INST_COUNT[]  = "IC";
static const char STR_GROUPPED[]    = "GROUPPED";
static const char STR_SELECTED[]    = "SELECTED";
static const char STR_NODESET[]     = "NODESET";
static const char STR_GROUP_NAME[]  = "GROUPNAME";
static const char STR_PATHINFO[]    = "PATHINFO";
static const char STR_SIMILARINFO[] = "SIMILARINFO";

//--------------------------------------------------------------------------
//--  NODEGROUP_LIST CLASS  ------------------------------------------------
//--------------------------------------------------------------------------
void nodegroup_list_t::free_nodegroup(bool free_nodes)
{
  for (iterator it=begin(); it != end(); ++it)
  {
    pnodegroup_t ng = *it;
    if (free_nodes)
      ng->free_nodes();

    delete ng;
  }
}

//--------------------------------------------------------------------------
pnodedef_t nodegroup_list_t::get_first_node()
{
  if (empty())
    return NULL;

  pnodegroup_t ng = *begin();
  return ng->get_first_node();
}

//--------------------------------------------------------------------------
pnodegroup_t nodegroup_list_t::get_first_ng()
{
  if (empty())
    return NULL;
  else
    return *begin();
}

//--------------------------------------------------------------------------
pnodegroup_t nodegroup_list_t::find_biggest()
{
  pnodegroup_t ng = NULL;
  for (iterator it=begin();
       it != end();
       ++it)
  {
    pnodegroup_t ng_f = *it;
    if (    ng == NULL 
      || ng->size() < ng_f->size())
    {
      ng = ng_f;
    }
  }
  return ng;
}

//--------------------------------------------------------------------------
pnodegroup_t nodegroup_list_t::add_nodegroup(pnodegroup_t ng)
{
  if (ng == NULL)
    ng = new nodegroup_t();

  push_back(ng);

  return ng;
}

//--------------------------------------------------------------------------
//--  NODEGROUP CLASS  -----------------------------------------------------
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
void nodegroup_t::free_nodes()
{
  for (iterator it=begin(); it != end(); ++it)
  {
    pnodedef_t nd = *it;
    delete nd;
  }
}

//--------------------------------------------------------------------------
pnodedef_t nodegroup_t::add_node(pnodedef_t nd)
{
  if (nd == NULL)
    nd = new nodedef_t();

  push_back(nd);
  return nd;
}

//--------------------------------------------------------------------------
pnodedef_t nodegroup_t::get_first_node()
{
  if (empty())
    return NULL;
  else
    return *begin();
}

//--------------------------------------------------------------------------
//--  SUPER GROUP CLASS  ---------------------------------------------------
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
supergroup_t::~supergroup_t()
{
  clear();
}

//--------------------------------------------------------------------------
supergroup_t::supergroup_t(): is_synthetic(false)
{
}

//--------------------------------------------------------------------------
pnodegroup_t supergroup_t::add_nodegroup(pnodegroup_t ng)
{
  if (ng == NULL)
    ng = new nodegroup_t();

  groups.push_back(ng);
  return ng;
}

//--------------------------------------------------------------------------
void supergroup_t::clear()
{
  groups.free_nodegroup(true);
  groups.clear();
}

//--------------------------------------------------------------------------
void supergroup_t::remove_nodegroup(pnodegroup_t ng, bool free_ng)
{
  groups.remove(ng);
  if (free_ng)
    delete ng;
}

//--------------------------------------------------------------------------
pnodedef_t supergroup_t::get_first_node()
{
  pnodegroup_t ng = get_first_ng();
  return ng->get_first_node();
}

//--------------------------------------------------------------------------
pnodegroup_t supergroup_t::get_first_ng()
{
  return groups.get_first_ng();
}

//--------------------------------------------------------------------------
void supergroup_listp_t::copy_to(psupergroup_t dest)
{
  //TODO: to deep copy one SG to another
}

//--------------------------------------------------------------------------
void supergroup_listp_t::remove_sg(psupergroup_t sg, bool free_sg)
{
  remove(sg);
  if (free_sg)
  {
    sg->clear();
    delete sg;
  }
}

//--------------------------------------------------------------------------
const char *supergroup_t::get_display_name(const char *defval)
{
  const char *s;
  if (name.empty())
  {
    if (id.empty())
      s = defval;
    else
      s = id.c_str();
  }
  else
  {
      s = name.c_str();
  }
  return s;
}

//--------------------------------------------------------------------------
void supergroup_t::copy_attr_from(psupergroup_t sg)
{
  static int ncopy = 1;
  this->id.sprnt("%s - copy_%d", sg->id.c_str(), ncopy);
  this->name.sprnt("%s - copy_%d", sg->name.c_str(), ncopy);

  this->is_synthetic = sg->is_synthetic = false;
  ++ncopy;
}

//--------------------------------------------------------------------------
//--  GROUP MANAGER CLASS  -------------------------------------------------
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
groupman_t::~groupman_t()
{
  clear();
}

//--------------------------------------------------------------------------
void groupman_t::clear()
{
  clear_sgl(&path_sgl);
  clear_sgl(&similar_sgl);
  all_nodes.clear();
}

//--------------------------------------------------------------------------
void groupman_t::clear_sgl(psupergroup_listp_t sgl)
{
  for (supergroup_listp_t::iterator it=sgl->begin(); 
       it != sgl->end();
       ++it)
  {
    psupergroup_t sg = *it;
    delete sg;
  }
  sgl->clear();
}

//--------------------------------------------------------------------------
nodeloc_t *groupman_t::find_nodeid_loc(int nid)
{
  nid2nloc_map_t::iterator it = nid2loc.find(nid);
  if (it == nid2loc.end())
    return NULL;
  else
    return &it->second;
}

//--------------------------------------------------------------------------
nodeloc_t *groupman_t::find_node_loc(ea_t ea)
{
  //HINT: use a map with lower_bound() if this function is to be called
  //      frequently and speed is of importance
  for (nid2ndef_t::iterator it = all_nodes.begin();
       it != all_nodes.end();
       ++it)
  {
    pnodedef_t nd = it->second;
    if (nd->start >= ea && ea < nd->end)
      return find_nodeid_loc(nd->nid);
  }
  return NULL;
}

//--------------------------------------------------------------------------
pnodedef_t groupman_t::get_first_nd()
{
  // No super groups defined?
  if (path_sgl.empty())
    return NULL;

  // Get the first super group
  psupergroup_t first_sgroup = (*(path_sgl.begin()));
  if (first_sgroup->gcount() == 0)
    return NULL;

  // Get groups count in the group
  if (first_sgroup->groups.empty())
    return NULL;

  // Get first group
  pnodegroup_t ng = *(first_sgroup->groups.begin());
  if (ng->empty())
  {
    // No nodes in the first group?
    return NULL;
  }
  else
  {
    // REturn the first node in the first group
    return *(ng->begin());
  }
}

//--------------------------------------------------------------------------
bool groupman_t::parse_nodeset(
      psupergroup_t sg,
      char *grpstr)
{
  // Find node group bounds
  for ( /*init*/ char *p_group_start = NULL, *p_group_end = NULL;
        /* cond*/(p_group_start = strchr(grpstr, '(')) != NULL
             && (p_group_start = skipSpaces(p_group_start+1), (p_group_end = strchr(p_group_start, ')')) != NULL);
        /*incr*/)
  {
    // Terminate the string with the closing parenthesis
    *p_group_end = '\0';

    // Advance to next group
    grpstr = skipSpaces(p_group_end + 1);

    // Add a new group
    pnodegroup_t ng = sg->add_nodegroup();

    for (/*init*/ char *saved_ptr, 
                  *p = p_group_start, 
                  *token = qstrtok(p, ",", &saved_ptr);
         /*cond*/ p != NULL;
         /*incr*/ p = qstrtok(NULL, ",", &saved_ptr))
    {
      p = skipSpaces(p);

      int nid;
      ea_t start = 0, end = 0;
      if (qsscanf(p, "%d : %a : %a", &nid, &start, &end) <= 0)
        continue;

      // Create an ND
      nodedef_t *nd = ng->add_node();
      nd->nid = nid;
      nd->start = start;
      nd->end = end;

      // Map this node
      map_nodedef(nid, nd);
    }
  }
  return true;
}

//--------------------------------------------------------------------------
void groupman_t::initialize_lookups()
{
  // Clear previous cache structures
  nid2loc.clear();

  // Build new cache
  for (supergroup_listp_t::iterator it=path_sgl.begin();
       it != path_sgl.end();
       ++it)
  {
    // Walk each super group
    psupergroup_t sg = *it;
    for (nodegroup_list_t::iterator it=sg->groups.begin();
         it != sg->groups.end();
         ++it)
    {
      // Walk each group contents
      pnodegroup_t ng = *it;
      for (nodegroup_t::iterator it=ng->begin();
           it != ng->end();
           ++it)
      {
        // Grab each node def
        nodedef_t *nd = *it;
        
        // Remember where this node is located
        nid2loc[nd->nid] = nodeloc_t(sg, ng, nd);
      }
    }
  }
}

//--------------------------------------------------------------------------
psupergroup_t groupman_t::add_supergroup(
    psupergroup_listp_t sgl,
    psupergroup_t sg)
{
  if (sgl == NULL)
    sgl = get_path_sgl();

  if (sg == NULL)
    sg = new supergroup_t();

  sgl->push_back(sg);
  return sg;
}

//--------------------------------------------------------------------------
void groupman_t::remove_supergroup(
      psupergroup_listp_t sgl,
      psupergroup_t sg)
{
  sgl->remove(sg);
}

//--------------------------------------------------------------------------
pnodegroup_t groupman_t::combine_ngl(pnodegroup_list_t ngl)
{
  // Get the biggest group and use as the destination container
  pnodegroup_t dest_ng = ngl->find_biggest();

  if (dest_ng == NULL)
    return NULL;

  for (nodegroup_list_t::iterator it = ngl->begin();
       it != ngl->end(); 
       ++it)
  {
    pnodegroup_t ng = *it;

    // Skip the dest NG
    if (ng == dest_ng)
      continue;

    // Get the first node from the other NG
    pnodedef_t nd = ng->get_first_node();

    // Node group is empty?
    if (nd == NULL)
      continue;

    // Get the supergroup containing this node group
    nodeloc_t *loc = find_nodeid_loc(nd->nid);

    // Move all node definitions to the first node group
    for (nodegroup_t::iterator it = ng->begin();
         it != ng->end();
         ++it)
    {
      pnodedef_t nd = *it;
      dest_ng->add_node(nd);
    }

    // Clear the items in the node group
    ng->clear();

    // Remove this node group from the super group
    loc->sg->remove_nodegroup(ng, false);
    if (loc->sg->empty())
    {
      remove_supergroup(
        get_path_sgl(),
        loc->sg);
    }
  }

  // Reinitialize lookups
  initialize_lookups();

  return dest_ng;
}

//--------------------------------------------------------------------------
pnodegroup_t groupman_t::move_nodes_to_ng(pnodegroup_t ng)
{
  // Algo:
  // ------
  // Make a new NG
  // Get the SG of the first selected node
  // Remove each node from its NG
  // Remove the NG from the SG if it becomes empty
  // Remove SG if it becomes empty
  // Add the ND to the new NG
  // Add the new NG to the SG

  psupergroup_t sg0 = NULL;
  pnodegroup_t  new_ng = NULL;
  for (nodegroup_t::iterator it=ng->begin();
       it != ng->end();
       ++it)
  {
    nodeloc_t *loc = find_nodeid_loc((*it)->nid);

    // Get the first SG
    if (sg0 == NULL)
    {
      sg0 = loc->sg;
      // Make a new NG (once) and add it to the SG
      new_ng = sg0->add_nodegroup();
    }

    // Remove the node from its NG
    loc->ng->remove(loc->nd);

    // Empty NG? remove it
    if (loc->ng->empty())
    {
      loc->sg->remove_nodegroup(loc->ng, true);
      if (loc->sg->empty())
        path_sgl.remove_sg(loc->sg, true);
    }

    // Add the node to the new NG
    new_ng->add_node(loc->nd);
  }
  // Reinitialize lookups
  initialize_lookups();

  return new_ng;
}

//--------------------------------------------------------------------------
void groupman_t::emit_sgl(
    FILE *fp,
    psupergroup_listp_t sgl)
{
  for (supergroup_listp_t::iterator it=sgl->begin();
       it != sgl->end();
       ++it)
  {
    psupergroup_t sg = *it;

    // Write ID
    if (!sg->id.empty())
      qfprintf(fp, "%s:%s;", STR_ID, sg->id.c_str());

    // Write Name
    if (!sg->name.empty())
      qfprintf(fp, "%s:%s;", STR_GROUP_NAME, sg->name.c_str());

    size_t group_count = sg->groups.size();
    if (group_count > 0)
    {
      qfprintf(fp, "%s:", STR_NODESET);
      nodegroup_list_t &ngl = sg->groups;
      for (nodegroup_list_t::iterator it = ngl.begin(); 
           it != ngl.end(); 
           ++it)
      {
        pnodegroup_t ng = *it;

        qfprintf(fp, "(");

        size_t c = ng->size();
        for (nodegroup_t::iterator it = ng->begin();
             it != ng->end();
             ++it)
        {
          nodedef_t *nd = *it;
          qfprintf(fp, "%d : %a : %a", nd->nid, nd->start, nd->end);
          if (--c != 0)
            qfprintf(fp, ", ");
        }
        qfprintf(fp, ")");
        if (--group_count != 0)
          qfprintf(fp, ", ");
      }
    }
    qfprintf(fp, "\n");
  }
}

//--------------------------------------------------------------------------
bool groupman_t::emit(
        const char *filename, 
        const char *additional_sections)
{
  FILE *fp = qfopen(filename, "w");
  if (fp == NULL)
    return false;

  qfprintf(fp, "--%s\n", STR_PATHINFO);
  emit_sgl(fp, &path_sgl);

  qfprintf(fp, "--%s\n", STR_SIMILARINFO);
  emit_sgl(fp, &similar_sgl);

  // Emit additional sections
  if (additional_sections != NULL)
    qfprintf(fp, "%s\n", additional_sections);

  qfclose(fp);

  return true;
}

//--------------------------------------------------------------------------
bool groupman_t::parse_line(
    psupergroup_t sg,
    char *line)
{
  for (char *saved_ptr, *token = qstrtok(line, ";", &saved_ptr); 
       token != NULL;
       token = qstrtok(NULL, ";", &saved_ptr))
  {
    char *val = strchr(token, ':');
    if (val == NULL)
      continue;

    // Kill separator and adjust value pointer
    *val++ = '\0';
    val = skipSpaces(val);

    // Set key pointer
    char *key = skipSpaces(token);

    if (stricmp(key, STR_ID) == 0)
    {
      sg->id = val;  
    }
    else if (stricmp(key, STR_GROUP_NAME) == 0)
    {
      sg->name = val;
    }
    else if (stricmp(key, STR_NODESET) == 0)
    {
      if (!parse_nodeset(sg, val))
        return false;
    }
  }
  return true;
}

//--------------------------------------------------------------------------
bool groupman_t::parse(
    const char *filename, 
    bool init_cache)
{
  std::ifstream in_file(filename);
  if (!in_file.is_open())
    return false;

  // Remember the opened file name
  this->src_filename = filename;

  // Clear previous items
  clear();

  std::string line;

  //TODO: generate dummy group names ; int group_dummy_name;
  psupergroup_listp_t cur_sgl = &path_sgl;

  while (in_file.good())
  {
    // Read the line
    std::getline(in_file, line);

    // Skip comment or empty lines
    char *s = skipSpaces((char *)line.c_str());
    if (s[0] == '\0' || s[0] == '#')
      continue;

    // Section switch?
    if (s[0] == '-' && s[1] == '-' && s[2] != '\0')
    {
      s += 2;
      if (qstrcmp(s, STR_PATHINFO) == 0)
        cur_sgl = &path_sgl;
      else if (qstrcmp(s, STR_SIMILARINFO) == 0)
        cur_sgl = &similar_sgl;
      else
        cur_sgl = NULL;

      // Skip this line after section switch
      continue;
    }

    // Skip lines when no known SGL section is being parsed
    if (cur_sgl == NULL)
      continue;

    // Take a copy of the line so we tokenize it
    s = qstrdup(s);

    // Create a new super group definition per line
    psupergroup_t sg = add_supergroup(cur_sgl);

    parse_line(sg, s);

    // Free this line
    qfree(s);
  }
  in_file.close();

  // Initialize cache
  if (init_cache)
    initialize_lookups();

  return true;
}

//--------------------------------------------------------------------------
void groupman_t::reset_groupping()
{
  // ALGO
  // -------
  // TODO: The clear() and destructor is confusing and complicated. Simplify
  //
  // Go from SGL to SG to NGL to NG and clear references to node definitions
  psupergroup_listp_t sgl = &path_sgl;
  for (supergroup_listp_t::iterator it=sgl->begin(); 
       it != sgl->end();
       ++it)
  {
    psupergroup_t sg = *it;

    for (nodegroup_list_t::iterator it=sg->groups.begin();
         it != sg->groups.end();
         ++it)
    {
      pnodegroup_t ng = *it;
      // Clear the node definitions -> no more ND references
      ng->clear();
    }
  }
  
  // Properly clear the SGL which eventually have no nodes
  clear_sgl(sgl);

  // Now repopulate from all_nodes
  for (nid2ndef_t::iterator it=all_nodes.begin();
       it != all_nodes.end();
       ++it)
  {
    psupergroup_t sg = add_supergroup(sgl);  
    pnodegroup_t  ng = sg->add_nodegroup();
    pnodedef_t    nd = it->second;
    
    ng->add_node(nd);
    sg->id.sprnt("node%d", nd->nid);
  }

  // Reinitialize lookups
  initialize_lookups();
}
