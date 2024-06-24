// automatically generated by the FlatBuffers compiler, do not modify
import * as flatbuffers from 'flatbuffers';
import { TableInC } from '../namespace-c/table-in-c';
export class SecondTableInA {
    constructor() {
        this.bb = null;
        this.bb_pos = 0;
    }
    __init(i, bb) {
        this.bb_pos = i;
        this.bb = bb;
        return this;
    }
    static getRootAsSecondTableInA(bb, obj) {
        return (obj || new SecondTableInA()).__init(bb.readInt32(bb.position()) + bb.position(), bb);
    }
    static getSizePrefixedRootAsSecondTableInA(bb, obj) {
        bb.setPosition(bb.position() + flatbuffers.SIZE_PREFIX_LENGTH);
        return (obj || new SecondTableInA()).__init(bb.readInt32(bb.position()) + bb.position(), bb);
    }
    referToC(obj) {
        const offset = this.bb.__offset(this.bb_pos, 4);
        return offset ? (obj || new TableInC()).__init(this.bb.__indirect(this.bb_pos + offset), this.bb) : null;
    }
    static getFullyQualifiedName() {
        return 'NamespaceA.SecondTableInA';
    }
    static startSecondTableInA(builder) {
        builder.startObject(1);
    }
    static addReferToC(builder, referToCOffset) {
        builder.addFieldOffset(0, referToCOffset, 0);
    }
    static endSecondTableInA(builder) {
        const offset = builder.endObject();
        return offset;
    }
    static createSecondTableInA(builder, referToCOffset) {
        SecondTableInA.startSecondTableInA(builder);
        SecondTableInA.addReferToC(builder, referToCOffset);
        return SecondTableInA.endSecondTableInA(builder);
    }
    unpack() {
        return new SecondTableInAT((this.referToC() !== null ? this.referToC().unpack() : null));
    }
    unpackTo(_o) {
        _o.referToC = (this.referToC() !== null ? this.referToC().unpack() : null);
    }
}
export class SecondTableInAT {
    constructor(referToC = null) {
        this.referToC = referToC;
    }
    pack(builder) {
        const referToC = (this.referToC !== null ? this.referToC.pack(builder) : 0);
        return SecondTableInA.createSecondTableInA(builder, referToC);
    }
}
