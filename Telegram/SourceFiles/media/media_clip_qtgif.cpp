/*
This file is part of Telegram Desktop,
the official desktop version of Telegram messaging app, see https://telegram.org

Telegram Desktop is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

It is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

In addition, as a special exception, the copyright holders give permission
to link the code of portions of this program with the OpenSSL library.

Full license: https://github.com/telegramdesktop/tdesktop/blob/master/LICENSE
Copyright (c) 2014-2016 John Preston, https://desktop.telegram.org
*/
#include "stdafx.h"
#include "media/media_clip_qtgif.h"

namespace Media {
namespace Clip {
namespace internal {

QtGifReaderImplementation::QtGifReaderImplementation(FileLocation *location, QByteArray *data) : ReaderImplementation(location, data) {
}

bool QtGifReaderImplementation::readNextFrame() {
	if (_reader) _frameDelay = _reader->nextImageDelay();
	if (_framesLeft < 1 && !jumpToStart()) {
		return false;
	}

	_frame = QImage(); // QGifHandler always reads first to internal QImage and returns it
	if (!_reader->read(&_frame) || _frame.isNull()) {
		return false;
	}
	--_framesLeft;
	return true;
}

bool QtGifReaderImplementation::renderFrame(QImage &to, bool &hasAlpha, const QSize &size) {
	t_assert(!_frame.isNull());
	if (size.isEmpty() || size == _frame.size()) {
		int32 w = _frame.width(), h = _frame.height();
		if (to.width() == w && to.height() == h && to.format() == _frame.format()) {
			if (to.byteCount() != _frame.byteCount()) {
				int bpl = qMin(to.bytesPerLine(), _frame.bytesPerLine());
				for (int i = 0; i < h; ++i) {
					memcpy(to.scanLine(i), _frame.constScanLine(i), bpl);
				}
			} else {
				memcpy(to.bits(), _frame.constBits(), _frame.byteCount());
			}
		} else {
			to = _frame.copy();
		}
	} else {
		to = _frame.scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	}
	hasAlpha = _frame.hasAlphaChannel();
	_frame = QImage();
	return true;
}

int QtGifReaderImplementation::nextFrameDelay() {
	return _frameDelay;
}

bool QtGifReaderImplementation::start(Mode mode) {
	if (mode == Mode::OnlyGifv) return false;
	return jumpToStart();
}

QtGifReaderImplementation::~QtGifReaderImplementation() {
	deleteAndMark(_reader);
}

bool QtGifReaderImplementation::jumpToStart() {
	if (_reader && _reader->jumpToImage(0)) {
		_framesLeft = _reader->imageCount();
		return true;
	}

	delete _reader;
	initDevice();
	_reader = new QImageReader(_device);
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
	_reader->setAutoTransform(true);
#endif
	if (!_reader->canRead() || !_reader->supportsAnimation()) {
		return false;
	}
	_framesLeft = _reader->imageCount();
	if (_framesLeft < 1) {
		return false;
	}
	return true;
}

} // namespace internal
} // namespace Clip
} // namespace Media
