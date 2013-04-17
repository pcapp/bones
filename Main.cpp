#include <iostream>
#include <fstream>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

using std::cout;
using std::endl;
using std::ifstream;

void loadMeshesData(xmlDocPtr doc) {
	xmlChar *xpath = (xmlChar *)"//x:library_geometries/x:geometry";
	xmlXPathContextPtr context = xmlXPathNewContext(doc);

	if(!context) {
		cout << "Error creating the XPath context." << endl;
		return;
	}

	xmlXPathRegisterNs(context, (const xmlChar *)"x", (const xmlChar *)"http://www.collada.org/2005/11/COLLADASchema");
	xmlXPathObjectPtr results = xmlXPathEvalExpression(xpath, context);
	xmlXPathFreeContext(context);
	context = nullptr;

	if(!results) {
		cout << "Error with xmlXPathEvalExpression" << endl;
		return;
	}

	cout << "Empty? " << xmlXPathNodeSetIsEmpty(results->nodesetval) << endl;

	xmlNodeSetPtr nodeset = results->nodesetval;

	cout << "Found " << nodeset->nodeNr << " nodes." << endl;
}

int main() {
	cout << "Hello, animated character!" << endl;
	const char *filename = "Character.dae";

	xmlDocPtr doc = nullptr;

	doc = xmlParseFile(filename);
	if(!doc) {
		cout << "Could not parse " << filename << endl;
		return -1;
	}	

	cout << "Loaded the file!" << endl;
	
	xmlNodePtr cur = xmlDocGetRootElement(doc);
	cout << "Root name: " << cur->name << endl;

	loadMeshesData(doc);

	xmlFreeDoc(doc);
	xmlCleanupParser();

	return 0;
}