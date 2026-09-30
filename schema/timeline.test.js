import { test } from "node:test";
import assert from "node:assert/strict";
import { readFileSync, readdirSync } from "node:fs";
import Ajv2020 from "ajv/dist/2020.js";
import addFormats from "ajv-formats";

const readJson = (url) => JSON.parse(readFileSync(url, "utf8"));
const fixtureDir = (kind) => new URL(`./fixtures/${kind}/`, import.meta.url);
const fixtureNames = (kind) =>
  readdirSync(fixtureDir(kind)).filter((name) => name.endsWith(".json")).sort();
const readFixture = (kind, name) => readJson(new URL(name, fixtureDir(kind)));

const ajv = new Ajv2020({ allErrors: true });
addFormats(ajv);
const validateSchema = ajv.compile(
  readJson(new URL("./timeline.schema.json", import.meta.url)),
);

// t の先頭 0・単調増加は JSON Schema で表現できないため、ここで検査する
function frameTimingErrors(frames) {
  const errors = [];
  if (frames[0].t !== 0) {
    errors.push(`frames[0].t must be 0, got ${frames[0].t}`);
  }
  for (let i = 1; i < frames.length; i++) {
    if (frames[i].t <= frames[i - 1].t) {
      errors.push(
        `frames[${i}].t (${frames[i].t}) must be greater than frames[${i - 1}].t (${frames[i - 1].t})`,
      );
    }
  }
  return errors;
}

function validateTimeline(timeline) {
  if (!validateSchema(timeline)) {
    return validateSchema.errors.map((e) => `${e.instancePath} ${e.message}`);
  }
  return frameTimingErrors(timeline.frames);
}

for (const name of fixtureNames("valid")) {
  test(`valid/${name} is accepted`, () => {
    assert.deepEqual(validateTimeline(readFixture("valid", name)), []);
  });
}

const invalidCases = [
  { name: "frames-over-limit.json", expectedError: "/frames must NOT have more than 200 items" },
  { name: "t-not-monotonic.json", expectedError: "frames[2].t (300) must be greater than frames[1].t (600)" },
  { name: "t-starts-nonzero.json", expectedError: "frames[0].t must be 0, got 300" },
];

test("every invalid fixture has an expected error", () => {
  assert.deepEqual(fixtureNames("invalid"), invalidCases.map((c) => c.name).sort());
});

for (const { name, expectedError } of invalidCases) {
  test(`invalid/${name} is rejected`, () => {
    const errors = validateTimeline(readFixture("invalid", name));
    assert.ok(
      errors.some((e) => e.includes(expectedError)),
      `expected "${expectedError}" in:\n${errors.join("\n")}`,
    );
  });
}
