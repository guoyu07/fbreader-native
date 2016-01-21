/*
 * Copyright (C) 2004-2015 FBReader.ORG Limited <contact@fbreader.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <JSONWriter.h>
#include <JSONUtil.h>

#include "BookModel.h"
#include "ModelWriter.h"
#include "../library/Book.h"

void ModelWriter::writeModel(const ZLTextModel &model, shared_ptr<JSONMapWriter> writer) {
	writer->addElementIfNotEmpty("id", model.id());
	writer->addElementIfNotEmpty("lang", model.language());
	writer->addElement("size", model.paragraphsNumber());
	const ZLCachedMemoryAllocator &allocator = model.allocator();
	writer->addElement("ext", allocator.fileExtension());
	writer->addElement("blks", allocator.blocksNumber());
	JSONUtil::serializeIntArrayAsCounts(model.startEntryIndices(), writer->addArray("ei"));
	JSONUtil::serializeIntArrayAsDiffs(model.startEntryOffsets(), writer->addArray("eo"));
	JSONUtil::serializeIntArray(model.paragraphLengths(), writer->addArray("pl"));
	JSONUtil::serializeIntArrayAsDiffs(model.textSizes(), writer->addArray("ts"));
	JSONUtil::serializeByteArray(model.paragraphKinds(), writer->addArray("pk"));
}

void ModelWriter::writeInternalHyperlinks(BookModel &model, const std::string &cacheDir, shared_ptr<JSONMapWriter> writer) {
	ZLCachedMemoryAllocator allocator(131072, cacheDir, "nlinks");

	ZLUnicodeUtil::Ucs2String ucs2id;
	ZLUnicodeUtil::Ucs2String ucs2modelId;

	const std::map<std::string,BookModel::Label> &links = model.internalHyperlinks();
	std::map<std::string,BookModel::Label>::const_iterator it = links.begin();
	for (; it != links.end(); ++it) {
		const std::string &id = it->first;
		const BookModel::Label &label = it->second;
		if (label.Model.isNull()) {
			continue;
		}
		ZLUnicodeUtil::utf8ToUcs2(ucs2id, id);
		ZLUnicodeUtil::utf8ToUcs2(ucs2modelId, label.Model->id());
		const std::size_t idLen = ucs2id.size() * 2;
		const std::size_t modelIdLen = ucs2modelId.size() * 2;

		char *ptr = allocator.allocate(idLen + modelIdLen + 8);
		ZLCachedMemoryAllocator::writeUInt16(ptr, ucs2id.size());
		ptr += 2;
		std::memcpy(ptr, &ucs2id.front(), idLen);
		ptr += idLen;
		ZLCachedMemoryAllocator::writeUInt16(ptr, ucs2modelId.size());
		ptr += 2;
		std::memcpy(ptr, &ucs2modelId.front(), modelIdLen);
		ptr += modelIdLen;
		ZLCachedMemoryAllocator::writeUInt32(ptr, label.ParagraphNumber);
	}
	allocator.flush();

	writer->addElement("ext", allocator.fileExtension());
	writer->addElement("blks", allocator.blocksNumber());
}

static bool ct_compare(const shared_ptr<ContentsTree> &first, const shared_ptr<ContentsTree> &second) {
	return first->reference() < second->reference();
}

void ModelWriter::writeTOC(const ContentsTree &tree, shared_ptr<JSONMapWriter> writer) {
	const std::string &text = tree.text();
	writer->addElementIfNotEmpty("t", text);
	const int ref = tree.reference();
	if (ref >= 0) {
		writer->addElement("r", ref);
	}
	std::vector<shared_ptr<ContentsTree> > children = tree.children();
	if (children.size() > 0) {
		shared_ptr<JSONArrayWriter> childrenWriter = writer->addArray("c");
		std::sort(children.begin(), children.end(), ct_compare);
		for (std::vector<shared_ptr<ContentsTree> >::const_iterator it = children.begin(); it != children.end(); ++it) {
			writeTOC(**it, childrenWriter->addMap());
		}
	}
}
