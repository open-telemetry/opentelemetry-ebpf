// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

package io.opentelemetry.render.extensions

import io.opentelemetry.render.render.FieldType
import io.opentelemetry.render.render.FieldTypeEnum

class FieldTypeExtensions {

  static def cType(FieldType fieldType, boolean packedStrings) {
    switch(fieldType.enum_type) {
    case FieldTypeEnum.U8:  "uint8_t"
    case FieldTypeEnum.U16:  "uint16_t"
    case FieldTypeEnum.U32:  "uint32_t"
    case FieldTypeEnum.U64:  "uint64_t"
    case FieldTypeEnum.U128: "unsigned __int128"
    case FieldTypeEnum.S8:  "int8_t"
    case FieldTypeEnum.S16:  "int16_t"
    case FieldTypeEnum.S32:  "int32_t"
    case FieldTypeEnum.S64:  "int64_t"
    case FieldTypeEnum.S128:  "__int128"
    case FieldTypeEnum.STRING :
      if (packedStrings)
        "uint16_t"
      else
        "struct jb_blob"
    }
  }

  // |arraySize| is -1 if not an array
  static def cType(FieldType type, int arraySize) {
    val nonArrayType =
      if (type.isShortString) {
        '''short_string<«type.size»>'''
      } else {
        type.enum_type.literal
      }

    if (arraySize >= 0) {
      '''std::array<«nonArrayType»,«arraySize»>'''
    } else {
      nonArrayType
    }
  }

  static def wireCType(FieldType fieldType) {
    fieldType.cType(true)
  }

  static def parsedCType(FieldType fieldType) {
    fieldType.cType(false)
  }

  static def size(FieldType fieldType, boolean packedStrings) {
    if (fieldType.isIsShortString)
      fieldType.size
    else switch(fieldType.enum_type) {
      case FieldTypeEnum.U8:  1
      case FieldTypeEnum.U16:  2
      case FieldTypeEnum.U32:  4
      case FieldTypeEnum.U64:  8
      case FieldTypeEnum.U128:  16
      case FieldTypeEnum.S8:  1
      case FieldTypeEnum.S16:  2
      case FieldTypeEnum.S32:  4
      case FieldTypeEnum.S64:  8
      case FieldTypeEnum.S128:  16
      case FieldTypeEnum.STRING :
        if (packedStrings)
          2
        else
          16
    }
  }

  static def wireSize(FieldType fieldType) {
    fieldType.size(true)
  }

  static def parsedSize(FieldType fieldType) {
    fieldType.size(false)
  }

  /**
   * Get field alignment
   */
  static def alignment(FieldType fieldType, boolean packedStrings) {
    if (fieldType.isIsShortString)
      1
    else switch(fieldType.enum_type) {
      case FieldTypeEnum.U8:  1
      case FieldTypeEnum.U16:  2
      case FieldTypeEnum.U32:  4
      case FieldTypeEnum.U64:  8
      case FieldTypeEnum.U128:  16
      case FieldTypeEnum.S8:  1
      case FieldTypeEnum.S16:  2
      case FieldTypeEnum.S32:  4
      case FieldTypeEnum.S64:  8
      case FieldTypeEnum.S128:  16
      case FieldTypeEnum.STRING:
        if (packedStrings)
          2
        else
          8
    }
  }

  static def wireAlignment(FieldType fieldType) {
    fieldType.alignment(true)
  }

  static def parsedAlignment(FieldType fieldType) {
    fieldType.alignment(false)
  }

  static def isSigned(FieldType fieldType) {
    switch (fieldType.enum_type) {
    case FieldTypeEnum.S8,
    case FieldTypeEnum.S16,
    case FieldTypeEnum.S32,
    case FieldTypeEnum.S64,
    case FieldTypeEnum.S128:
      true
    default:
      false
    }
  }

  static def isInt(FieldType fieldType) {
    return (!fieldType.isShortString) &&
        (fieldType.enum_type != FieldTypeEnum.STRING)
  }

}
