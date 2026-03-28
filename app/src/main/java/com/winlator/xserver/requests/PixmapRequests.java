package com.winlator.xserver.requests;

import com.winlator.xconnector.XInputStream;
import com.winlator.xconnector.XOutputStream;
import com.winlator.xserver.Drawable;
import com.winlator.xserver.Pixmap;
import com.winlator.xserver.XClient;
import com.winlator.xserver.errors.BadDrawable;
import com.winlator.xserver.errors.BadIdChoice;
import com.winlator.xserver.errors.XRequestError;

public abstract class PixmapRequests {
    public static void createPixmap(XClient client, XInputStream inputStream, XOutputStream outputStream) throws XRequestError {
        byte depth = client.getRequestData();
        int pixmapId = inputStream.readInt();
        int drawableId = inputStream.readInt();
        short width = inputStream.readShort();
        short height = inputStream.readShort();

        if (!client.isValidResourceId(pixmapId)) throw new BadIdChoice(pixmapId);

        Drawable drawable = client.xServer.drawableManager.getDrawable(drawableId);
        if (drawable == null) throw new BadDrawable(drawableId);

        Drawable backingStore = client.xServer.drawableManager.createDrawable(pixmapId, width, height, depth);
        if (backingStore == null) throw new BadIdChoice(pixmapId);
        // Pixmaps are offscreen backing stores — they should never trigger a screen
        // refresh on their own. Without this flag, forceUpdate() fires on every
        // PutImage/CopyArea to a pixmap and finds onDrawListener=null, producing
        // spurious "N had NULL onDrawListener" log warnings at high frequency.
        // Screen updates are triggered when the pixmap's content is CopyArea'd to
        // a window drawable (which DOES have an onDrawListener).
        backingStore.setOffscreenStorage(true);
        Pixmap pixmap = client.xServer.pixmapManager.createPixmap(backingStore);
        if (pixmap == null) throw new BadIdChoice(pixmapId);
        client.registerAsOwnerOfResource(pixmap);
    }

    public static void freePixmap(XClient client, XInputStream inputStream, XOutputStream outputStream) throws XRequestError {
        client.xServer.pixmapManager.freePixmap(inputStream.readInt());
    }
}
